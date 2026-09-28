# Pinball Illusions reverse-engineering notes

These notes describe the installed GOG release `2.1.0.19`. All offsets are
hexadecimal unless stated otherwise. No original game data belongs in this
repository.

## Distribution layout

The Windows package launches DOSBox 0.74 and mounts `game.inst`, a cue sheet.
Track 1 is `game.gog`, a raw Mode 2 CD-ROM image with 2,352-byte physical
sectors and a 2,048-byte user-data area beginning 24 bytes into each sector.
Tracks 2 through 51 are OGG files used as Red Book audio.

The ISO 9660 volume is named `ILLUSION_CD_ROM` and contains:

| File | LBA | Size |
| --- | ---: | ---: |
| `ILLUSION.EXE` | 266 | 49,171,271 |
| `INSTALL.EXE` | 24,276 | 59,015 |

Observed SHA-256 values for identifying this release locally:

| File | SHA-256 |
| --- | --- |
| extracted `ILLUSION.EXE` | `11A1494CB508D8B7C2C358C4CE2ED1EA4B7316B97FDBB4830540E1336A751031` |
| extracted `INSTALL.EXE` | `59298E23A9F43F494B98C7B42B8D9292C94EEEBDD09AA0EF19B04A03A0D90859` |
| installed `ILLUSION.CFG` | `99D182FF28CDC59874AFAAEB4248BB3DACFC8E21FF80A90B1FBB4452262935BC` |
| decoded `ILLUSION.386` | `5932E424DE4F82CB7A0D55261B791C0F2D3391D1A18B4E77A104DA36806667B8` |

## Executable wrapper and pMAX

`ILLUSION.EXE` begins with a 23,632-byte (`0x5C50`) DOS MZ loader. Its body is
self-decoded at startup. The decoded loader identifies itself as:

`pMAX ver 1.30 (c) Copyright 1995 - FrontLine Design`

It supplies a 32-bit protected-mode runtime using DPMI or VCPI, installs
software interrupt APIs `0x90` through `0x94`, manages selectors and memory,
and implements the resource library and decompressor. The loader's assembled
date is 1995-08-15.

The loader mounts the MZ overlay as a resource library and asks it for
`ILLUSION.386`, the protected-mode main program. The large EXE is therefore a
small DOS extender plus an appended asset archive.

## Resource archive

The archive starts at MZ offset `0x5C50` and ends exactly at EOF. Its header is:

```c
struct archive_header {
    uint16_t entry_count;      /* 125 in the GOG image */
    uint16_t directory_bytes;  /* 0x0EFF */
};
```

The variable-length directory immediately follows. Each entry contains:

```c
struct archive_entry {
    uint8_t encoded_name[]; /* NUL terminated; each byte is ROL8(clear, 3) */
    uint32_t payload_offset;
    uint32_t unpacked_size;
    uint32_t packed_size;
};
```

Offsets are relative to the archive header. Each payload is
`uint32_t unpacked_size_again` followed by `packed_size` bytes. The next
payload begins at `offset + 4 + packed_size`. The first payload begins at
relative offset `0x0F03`, exactly after the four-byte archive header and the
`0x0EFF`-byte directory.

Names are compared case-insensitively by the runtime. It uppercases the lookup
name and rotates stored bytes right by three bits while comparing. The 125
entries include chooser/intro data, four sets of table assets, sound drivers,
and `ILLUSION.386` as the final entry.

## NLZW compression

The loader calls the format `NLZW`. It combines conventional LZW dictionary
reconstruction with a uniform arithmetic coder over the currently valid code
range.

- The initial dictionary has codes 0 through 255.
- The arithmetic alphabet starts at 256 and grows after every decoded code.
- A dictionary item contains a parent code and final byte.
- The usual LZW `code == next_code` special case is supported.
- At code `0x1F3F`, the dictionary and arithmetic alphabet reset to 256.
- Arithmetic state is 32-bit inclusive `low`, `high`, and `code`.
- Compressed words are read most-significant-bit first and byte-swapped from
  the little-endian DOS host representation.
- pMAX batches both common-prefix normalization and E3 underflow bits. Keeping
  its deferred-underflow behavior is necessary; a textbook arithmetic decoder
  diverges after the first few symbols.

The decoder (tools/illfiles.py) is checked by two format invariants:
`INTRO\\PCSKY.FLD` begins with a VGA palette whose channels are all 0 through
63, and `DATA\\S001\\MUSIC.MOD` contains the ProTracker `M.K.` signature at
offset 1080.

## `ILLUSION.386`

After NLZW decoding, the final resource is a pMAX flat-image executable:

```c
struct pmax_image_header {
    uint32_t primary_base;       /* 0 */
    uint32_t allocation_size;    /* 0x46480 */
    uint8_t  format_version;     /* 1 */
    uint8_t  descriptor_count;   /* 2 */
    uint32_t image_size;         /* 0x46480 */
    uint32_t entry_offset;       /* 0x2A3 */
    uint16_t relocation_count;   /* 24 */
};
```

The fixed header is 20 bytes. Two `{ uint32_t base, uint32_t size }` descriptor
records follow at file offset `0x14`, so the flat image begins at `0x24`. The
24 five-byte relocation records follow the image at `0x464A4`. A relocation is
a 32-bit image offset followed by an 8-bit descriptor index; the offset names
a 16-bit selector operand that the loader replaces with the descriptor's
runtime selector. Total decoded size is 288,028 bytes (`0x4651C`), exactly
matching header, descriptors, image, and relocations.

The image is raw 32-bit x86 based at zero. Its entry point is image offset
`0x2A3` (file offset `0x2C7`). The first relocation is at image offset `0x2A5`,
the immediate selector operand of the entry point's initial `mov ax, 0`.
An earlier tool (`pmax_inspect`, no longer in the repository) validated this
layout and printed every descriptor and selector relocation.

Relocations `0x6191` through `0x61CD` and `0x9902` through `0x993E` are
six bytes apart. They are the selector fields in two tables of 11 far pointers,
each encoded as a 32-bit image offset and a 16-bit selector. The final
relocation, at `0xA314`, is another immediate selector operand used while
switching data segments. All 24 records refer to descriptor zero and all the
on-disk selector words are zero.

### Observed entry-point flow

The entry point provides an initial map for further static analysis. These
labels describe observed behavior and are intentionally provisional:

1. Install descriptor zero as `DS`, then save that selector at `0x2A1`.
2. Query available memory through pMAX interrupt `0x92`, service `AH=0x0B`,
   and require at least `0x2F0800` bytes.
3. Obtain the command line through interrupt `0x93`, service `AH=0x11`, copy
   the executable name to `0x86C`, and open it as a resource library through
   interrupt `0x94`, service `AH=0x08`.
4. Call `0x32F39`, which reads the command line again and parses slash-prefixed
   options.
5. Initialize saved configuration state through `0x753`, mask keyboard IRQ 1
   at PIC port `0x21`, and enter the VGA setup at `0x49A`. That routine starts
   with BIOS mode `0x13` and then programs VGA CRTC registers directly.
6. Initialize the chooser/menu path at `0x4CFB`, sound at `0x7182`, and other
   shared state before entering the table-selection loop.
7. On exit, restore the keyboard IRQ mask, print the message at `0x7B4`, and
   return far to the pMAX loader.

The main image is 32-bit x86 code with extensive direct VGA, PIC, keyboard,
timer, and VESA access. Startup requires about 3.0 MiB, loads the external
configuration, initializes graphics and sound, then enters a table/menu loop.
The game advertises 320x240, 360x350, 640x480, and 800x600 paths. Embedded
credits identify the PC port as primarily assembly: approximately 2.9 MiB of
assembly source, 60 KiB of C, and 7,917 bytes of BASIC.

## Asset groups and likely subsystem boundaries

Each table (`S001` through `S004`) has a consistent asset family:

- `STAGE.M`: 230,400-byte playfield bitmap (320 x 720 at 8 bpp).
- `STAGE.C`: 768-byte RGB888 palette (256 three-byte entries).
- `MASK1`, `MASK2`, `HIDE1.M`, `HIDE2.M`, `PIXELS1`, `PIXELS2`, `ANGLE1`,
  `ANGLE2`: collision/occlusion/rendering data.
- `ANIMS/ALLANIMS.MGL` and mask MGL files: animated and masked graphics.
- `SOURCE/T00x.BPC` and `SOURCE/T00x.REL`: table-specific native program plus
  base relocations.
- `LINK1`, `LINK2`: fixed-size 4,800-byte linkage tables.
- `MUSIC.MOD` and sometimes `MUSIC2.MOD`: tracker music used when CD audio is
  unavailable or for in-game sequencing.
- `SPECIAL/VM_DATA.MGL`: table-specific virtual-machine/display data.

Large `FILE*.DAT` resources expand under NLZW and probably contain sample or
stream data that was already compressed before entering the archive.

`T001.BPC` is a `0x12000`-byte mixed code/data image containing 32-bit x86
instructions, resource references, display text, scoring modes, multiball,
jailbreak, riot, bomb, and other Law 'n Justice rules. `T001.REL` is exactly
1,707 little-endian 32-bit offsets (`6,828` bytes); its entries point at words
inside the BPC image that need adjustment when the module is loaded. This
confirms that the main image is the shared platform/physics/rendering engine
while substantial table rules are dynamically loaded native modules. The
`VM_DATA.MGL` resources are therefore likely for dot-matrix mini-games or
effects, not the top-level table-rule interpreter.

## Next reverse-engineering targets

Completed foundations:

- The flat image layout, entry point, descriptors, and all 24 selector
  relocations were validated by an earlier tool (`pmax_inspect`, removed).
- An earlier scanner (`x86_inventory.py`, removed) inventoried reachable
  instructions, software interrupts, hardware I/O, and unresolved indirect
  dispatches; it reached 12,018 bytes of the image (run 2026-09-29).
- The first pMAX, graphics, resource, digital-audio, CD-audio, input, and timer
  boundaries are recorded in [main-image-map.md](main-image-map.md).
- The host-visible layouts of the slot-14 light groups, slot-15 timed effects,
  slot-16 progression counters, slot-17 weighted choices, and slot-26 bounded
  BCD accumulators are validated by `bpc_inspect.py` and documented in
  [bpc-module.md](bpc-module.md).
- The module-side links from slot 14 and slot 16 to queued event sequences are
  validated, slot-15 `+0x10` is tied to the slot-31 display-state layout, and
  the slot-26 start/current/limit/step BCD fields are distinguished.
- The packed inputs to SDR commands `0x07`, `0x08`, and `0x0A`, their shared
  59-byte voice and 629-byte module strides, and the temporary-playback context
  save are validated across all eleven regular decoded drivers.
- The sound-driver callbacks identify pMAX services `0x93/03` and `0x93/04`
  as protected-mode interrupt-vector get/set operations. Main-image and
  `THING.SDR` consumers identify `0x93/0C` as a zero-based flat selector.

Next targets:

1. Dynamically confirm the statically mapped SDR voice/module parameters and
   temporary-playback resume behavior; the 19 commands and eleven host
   callbacks are mapped in [audio-driver.md](audio-driver.md).
2. Record deterministic input/output traces from DOSBox for ball integration,
   collision response, scrolling, scoring, and table VM execution.
3. Give the slot-15 display-state link and slot-26 eight-byte trailer narrower
   gameplay names if dynamic traces expose behavior beyond their now-validated
   structure.

The configuration container and all six visible runtime option fields are
mapped in [configuration.md](configuration.md), cross-checked against the
English option strings in `SETSOUND\\SETSOUND.DAT`.

The core `LINK*`, `PIXELS*`, `MASK*`, `HIDE*`, `ANGLE*`, `STAGE.M`, and
`STAGE.C` layouts are now characterized in
[asset-formats.md](asset-formats.md).



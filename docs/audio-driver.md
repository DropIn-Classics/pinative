# Dynamically loaded sound driver

This map is derived from the decoded resources in the GOG release and their
call sites in the decoded `ILLUSION.386` flat image. Names remain descriptive
where the binary does not provide symbols.

The configured driver name is the first NUL-terminated field in
`ILLUSION.CFG`. In the inspected installation it is `SB16.SDR`.

## Container and entry

An SDR resource is a directly loaded 32-bit x86 image, not a pMAX executable.
Byte zero is an `E9 rel32` jump over an embedded shared banner and driver data.
For `SB16.SDR`, the jump target is `0xEEA`. The main image calls offset zero;
it does not call `0x409A`. That value is passed to pMAX service `0x93/0x08`
while creating the callable selector.

At `SB16.SDR:0xEEA`, the driver saves all general and segment registers,
checks EAX, loads its own data selector for nonzero commands, and dispatches
through the dword table at `0xF5B`. Commands `0x00` through `0x12` have table
entries. The comparison admits `0x13`, but index `0x13` reads the first four
bytes of the following handler as a pointer. The same off-by-one boundary is
present in every structurally valid SDR inspected, so callers must never issue
command `0x13`.

Run the reproducible structural validator with:

```sh
python3 tools/sdr_inspect.py build/files/SB16.SDR
```

Eleven decoded drivers share this dispatcher and its 19 commands:
`ADLIB`, `GUS`, `INTERNAL`, `NOSOUND`, `PAS16`, `SB16`, `SB20`, `SBLASTER`,
`SBPRO`, `SM2`, and `THING`. Handler offsets vary, but their instruction
shapes and state transitions align. `SNDSCAPE.SDR` is anomalous in the
inspected release: its offset-zero jump targets `0x123D`, where the decoded
bytes are not a valid instance of this dispatcher. The archive stream decodes
cleanly to its declared 16,095 bytes, so the tool rejects that resource rather
than guessing a repair.

## Command ABI

The table below combines the aligned `SB16` and `NOSOUND` handlers with actual
main-image call sites. A behavior description is intentionally preferred over
an unsupported original function name.

| EAX | `SB16` handler | Observed inputs and behavior |
| ---: | ---: | --- |
| `0x00` | `0x0FA7` | Initialize. Captures the caller's `DS`, `ES:EBX` state pointer, `FS:EDI` 11-entry callback table, then initializes common and device-specific state. Carry reports failure. |
| `0x01` | `0x10E8` | Start the mixer/playback service. `CX` is clamped to 2..15, internal voices are initialized, and the device interrupt path is installed. The main image passes `CX=15`. |
| `0x02` | `0x1212` | Toggle the byte at `+0x123C` and pause or resume: `SB16` masks its DMA channel (`+0x8B0`) and sets the PIC masks from `+0x3306`/`+0x3307`, or unmasks it (`+0x8A2`) and restores the PIC masks. The main image issues it twice around each CD request (`CODE:9C4F`, `9C5E`); seen in runs 2026-09-29 (the byte 1 between the two calls, 0 after). |
| `0x03` | `0x123D` | Stop the active mixer/playback service and restore device state. Used during intro cleanup. |
| `0x04` | `0x12E2` | Load a module into slot `BL` from the caller-visible name at `ES:EDX`. The main image uses slot zero for `INTRO\\MOD.INT` and `CHOOSER\\MOD.MUS`. |
| `0x05` | `0x1330` | Release module slot `BL`. The main image releases slot zero before driver shutdown. |
| `0x06` | `0x1356` | Periodic service/update, issued every frame (`CODE:298EF`). It invokes the optional command-`0x11` callback when `+0x3322` is `0xFF` (a temporary playback ended, below) and clears it. |
| `0x07` | `0x138D` | Start one voice. `DL` is the one-based voice number, `CH` the loaded-module slot, `CL` the one-based instrument, and `BL` the one-based note. Nonzero `BH` is injected as the parameter of a tracker `Cxx` volume effect. |
| `0x08` | `0x14CF` | Start or reposition module playback. `CL` selects the loaded-module slot and `BL` its order-list index. The main image calls it with `BL=0x12`, `CL=0` during final cleanup. |
| `0x09` | `0x13E2` | Install a sample region from `FS:ESI` with byte count `ECX` into a selected voice record. |
| `0x0A` | `0x1576` | Start temporary module playback using the same `BL` order index and `CL` module slot as command `0x08`, after preserving the current module base, pattern cursor, order index, and playback-speed byte for later restoration. |
| `0x0B` | `0x1032` | Shut down the initialized driver after playback has stopped. |
| `0x0C` | `0x1270` | Set a global level from `BX` (to `+0x3820`), clamped to `0x100`. The chooser passes zero immediately after loading its music module; in a table the main image passes zero when a CD track starts and `0x100` before a jingle (`CODE:9C1A`, `9C34`). |
| `0x0D` | `0x1287` | Return a playback position derived from the service counter and active voice count. The main image compares the result with `0x280A`. |
| `0x0E` | `0x0BDC` | Store routine pointer `ES:EDX`, then run associated setup. The caller supplies the far-return routine at main-image offset `0x4C85`. |
| `0x0F` | `0x0BFA` | Store routine pointer `ES:EDX` and derive timer state from `CX`. The caller supplies offset `0x4C93` with `ECX=0x1999`. |
| `0x10` | `0x0C3D` | Program PIT channel 0 with divisor 5 in hardware drivers; `NOSOUND` implements this as a no-op. |
| `0x11` | `0x12B2` | Store optional completion callback `ES:EDX`, later invoked by command `0x06`. |
| `0x12` | `0x12CA` | Return byte field `+0x37` from the voice record selected by `DL` (one-based). |

All handlers return with `retf`. Normal completion clears carry. Error exits set
carry and preserve a small error code in EAX; several invalid-state paths use
codes zero through four.

## Voice and module-playback layout

The command implementations, not just their entry-table positions, have the
same shape in all eleven regular decoded drivers. Run the cross-driver check
with:

This was found with an earlier scanner (`sdr_voice.py`) that is no longer in the
repository (it is in build/earlier/ on the machine it was removed on);
to be checked again in the hints once the 32-bit stage 1 exists.

Command `0x07` subtracts one from `DL` and multiplies it by `0x3B`, establishing
a 59-byte internal voice-record stride. It multiplies the original `CH` by
`0x275`, establishing a 629-byte loaded-module slot. Before entering the shared
voice engine it transfers `CL` to the instrument register and `BL` to the note
register. The instrument path subtracts one and indexes 16-byte module entries;
the note path subtracts one and indexes a word frequency table. When `BH` is
nonzero, the wrapper supplies tracker effect number `0x0C` and uses `BH` as its
parameter; that effect clamps the voice level to `0x40`.

Commands `0x08` and `0x0A` both multiply `CL` by `0x275` and use `BL` to index
the selected module's order list. Command `0x0A` additionally copies four
active playback fields to a saved bank before selecting the temporary stream.
This statically identifies the packed parameters and the interruption/resume
boundary.

How a temporary playback ends (read in `SB16.SDR` 2026-09-29, seen in runs):
command `0x0A` sets `+0x3320` to `0xFF`. The `Bxx` effect (`+0x1EAF`, through
the tick-0 table at `+0x30E3`) compares its target with the order playing
(`+0x2773`) while `+0x3320` is set; equal, it sets `+0x3321` and `+0x3322` to
`0xFF` instead of jumping. At the next row the sequencer (`+0x2109`) restores
the saved module, pattern cursor, order and speed (`+0x15A6`'s copies, back by
`+0x15D1`, which clears `+0x3320` and `+0x3321`); command `0x06` calls the
completion callback and clears `+0x3322`.
So a jingle is the stretch of orders from the one asked for to a `Bxx` that
names the order it is in (table 1's `music2.mod` order `0x0E`: 1.69 s in a
run, `0x0F`: 2.71 s). The sequencer runs in the mixing loop (`+0x20E0`,
counting ticks at `+0x31F2`), so it stops when the card's transfer stops; an
`F00` (speed 0, `+0x201C`) sets `+0x31EE`, which holds the rows until a
command `0x08` or `0x0A`.

## Host callback table

Initialization receives `FS:EDI = 0x618D` in the chooser/intro path. The table
contains eleven six-byte far pointers. Its initializer replaces their selector
words with `CS`. Eleven callable entries are followed by a six-byte sentinel.

| Index | Chooser offset | Table offset | Observed host wrapper |
| ---: | ---: | ---: | --- |
| 0 | `0x6244` | `0x99B5` | Allocate named memory with pMAX `0x92/0x0A`. |
| 1 | `0x61ED` | `0x995E` | Temporarily select allocation policy 2, allocate named memory, then restore policy 0. |
| 2 | `0x6234` | `0x99A5` | Release the selector in `BX` with pMAX `0x92/0x05`. |
| 3 | `0x626D` | `0x99DE` | Replace selector `BX` with its linear base in `EBX` through pMAX `0x93/0x0B`. |
| 4 | `0x627B` | `0x99EC` | Get the protected-mode interrupt vector selected by `BL` through pMAX `0x93/0x03`; returns it in `ES:EDX`. |
| 5 | `0x6280` | `0x99F1` | Set the protected-mode interrupt vector selected by `BL` from `ES:EDX` through pMAX `0x93/0x04`. |
| 6 | `0x6285` | `0x99F6` | Load the resource named by the driver, save its selector and reset the copy cursor. |
| 7 | `0x62EE` | `0x9A5F` | Copy `ECX` bytes from the loaded resource cursor to `ES:EDI`, then advance the cursor. |
| 8 | `0x6318` | `0x9A89` | Release the saved loaded-resource selector. |
| 9 | `0x632E` | `0x9A9F` | Replace the resource copy cursor with `EDX`. |
| 10 | `0x633F` | `0x9AB0` | Obtain pMAX's zero-based flat selector through `0x93/0x0C` and return it in `BX`. |

Entry 11 is the `0xFFFFFFFF` terminator, not a callback. The two tables have
identical instruction shapes and differ only in their private cursor and
selector storage. Drivers use callbacks 4 and 5 to save, install, and restore
protected-mode interrupt vectors. `THING.SDR` exercises callback 10 directly:
it installs the returned `BX` selector in `FS` and reads BIOS Data Area offsets
`0x408` and `0x40C`. The main image independently uses the same pMAX result to
address the VGA BIOS at linear `0xC0000`, confirming that it is a zero-based
flat selector.

This was found with an earlier scanner (`pmax_services.py`) that is no longer in the
repository (it is in build/earlier/ on the machine it was removed on);
to be checked again in the hints once the 32-bit stage 1 exists.

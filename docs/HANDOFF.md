# Handoff

State of 2026-09-29: the repository was made from doskit's template on
top of an earlier analysis (docs/*.md other than this file). The game's
files are listed. The main program, `ILLUSION.386`, is in stage 1:
`src/ILLUSION.hints` rebuilds it byte for byte (doskit reads pMAX images
since 2026-09-29), with few hints yet (see "Stage 1" below).
The runner runs the game now (protected mode and MSCDEX in doskit,
2026-09-29) past its CD check through the intro, to a still picture at
about 100 s (see "The loader in the runner").

## The earlier analysis

Kept: its notes (docs/*.md) and its data-format tools with their
synthetic tests, `tools/bpc_inspect.py`, `asset_inspect.py`,
`sdr_inspect.py`, `cfg_inspect.py` (`python3 -m unittest discover -s
tests`: 45 tests ok, Python 3.9). Run on `build/files/` 2026-09-29 they
accept all four tables (`DATA\S00n`, `SOURCE\T00n.BPC`/`.REL`) and every
driver but `SNDSCAPE.SDR` ("no EAX jump-table dispatch", the driver the
notes already call anomalous); `cfg_inspect.py` is not run, there is no
`ILLUSION.CFG` here. `asset_inspect.py` now takes the archive's names
(`HIDE1.M`, `STAGE.C`, the old extractor wrote `HIDE1_M`).

Removed (not committed anywhere; moved to `build/earlier/` on the Mac
this was done on): the C extractor and pMAX inspector (`illfiles.py`
replaces the first), CMakeLists.txt, and the ad-hoc code scanners
(`x86_inventory`, `bpc_abi`, `bpc_events`, `bpc_header_xrefs`,
`pmax_services`, `sdr_voice`). What the notes say those scanners found
is marked there as to be checked again in the hints.

## Start here (next session)

- AGENTS.md and PROVENANCE.md have the rules: read them first.
- `game/` holds `ILLUSION.EXE` and `INSTALL.EXE`, the two files of the
  GOG image's ISO volume (SHA-256 in docs/reverse-engineering.md, checked
  against these copies 2026-09-29); it is not in the repository.
- `python3 tools/illfiles.py extract --all` first: the hints read
  `build/files/ILLUSION.386` (`pmax build/...`), so check.py and the
  pre-commit hook fail on a checkout where it is not unpacked.
- `python3 tools/illfiles.py list` lists the archive in ILLUSION.EXE,
  `extract --all` unpacks it into `build/files/` (80 s on the Mac used;
  all 125 entries come out at their directory sizes, 124 files as
  `FILE2.DAT` is there twice). `ILLUSION.386` comes out with the SHA-256
  in docs/reverse-engineering.md; `DATA\S001\MUSIC.MOD` has `M.K.` at
  1080, `INTRO\PCSKY.FLD` a palette of values up to 63. The other
  files' contents are not checked.

## The game's files

| Folder / file | What |
|---|---|
| `INSTALL.EXE` | the installer; 16-bit real mode, EXEPACK-packed (`RB` header at CS:0, "Packed file is corrupt"); German and English strings |
| `ILLUSION.EXE` | a 5310h-byte real-mode loader (pMAX 1.30, a DOS extender; no MZ relocations, its body decodes itself) followed at 5C50h by a resource archive of 125 entries up to the end of the file |
| archive `ILLUSION.386` | the main program: pMAX flat 32-bit image (docs/reverse-engineering.md) |
| archive `*.SDR` (12) | sound drivers, loaded by the main program (docs/audio-driver.md) |
| archive `SETSOUND\SETSOUND.DAT` | the sound set-up's option strings (docs/configuration.md) |
| archive `CHOOSER\` | table chooser: `*.RIX`, `INFODATA.MGL`, `TINYFONT.FNT`, `MOD.MUS` (music), `FILE1.DAT` |
| archive `INTRO\` | intro: `INTROANI.ROY`, `MOD.INT` (music), `INTROPIX.MGL`, `*.FLD`, `SCROLL.DLT` |
| archive `DATA\MISC\` | fonts `FONT*.M`, `FLIPDAT1.M`, `FILE2.DAT` |
| archive `DATA\S00n\` (n = 1..4) | one table each: `STAGE.M`/`.C`, `LINK*`, `PIXELS*`, `MASK*`, `HIDE*.M`, `ANGLE*`, `MUSIC*.MOD`, `ANIMS\`, `MASKS\`, `SPECIAL\` (docs/asset-formats.md), `FILEn.DAT` |
| archive `SOURCE\T00n.BPC`, `.REL` | each table's code module and its relocations (docs/bpc-module.md); `SOURCE\FILE8.DAT` |

Observed in the listing, not explained yet: the `FILEn.DAT` entries
(1.6 to 8 MB each, about 85 % of the archive) are larger packed than
unpacked, presumably data that was already compressed; `DATA\MISC\FILE2.DAT`
is in the archive twice (entries 44 and 89) with the same packed bytes;
which one the loader finds by name is not checked. Not checked either: whether the
installed game has `ILLUSION.CFG` (docs/configuration.md) outside the
archive, and what `cdadd.000` (a string in the loader) is.

## What does not fit the kit yet

doskit's stage 1 (disasm.py, tasm.py, build.py) and its runner were
16-bit real mode only when this began; both take 32-bit code now (see
"Next"). The game's code is 32-bit:

- `ILLUSION.386` and the `.SDR` drivers are flat 32-bit images run by
  pMAX's protected mode;
- the `.BPC` table modules are 32-bit code linked at load time;
- only the pMAX loader in `ILLUSION.EXE` and `INSTALL.EXE` are real mode,
  and both are packed or self-decoding.

Decided 2026-09-29: doskit gets 32-bit support (it is wanted for later
games too), in the kit with tests (rule 7).

## Stage 1: ILLUSION.386

`python3 doskit/tools/build.py src/ILLUSION.hints`: IDENTICAL, 29,973
instructions, 3912 labels, 17 lines as DB, 19 s on the Mac used
(2026-09-29); gaps.py: 210 gaps, 165,070 of 0x46480 bytes not reached as
code (see "The gaps" below). The hints so far: the two descriptors as segments (`CODE` the
whole image, code and data; `TAIL` the empty one at its end), the entry
point's name, the pointers found so far, a `stop` and the 17 `raw`
lines:

- twelve instructions with a 16-bit address and no register (67h, e.g.
  `mov word ptr es:[0x27], 0` at CODE:0667): a USE32 source line cannot
  ask for that address size;
- CODE:31649 `64 66 AD`: FS before 66h, the one such order;
- CODE:13AD7, 141D6, 164B4: a DS prefix where DS is the default, all
  three with EBP as the index register (45 others with it have none).

Code reached through pointers so far:

- CODE:A39C, where the program goes on after start-up: the code at CODE:A323 makes a
  selector with INT 93h AH=8 (CX=200000h) and jumps there with a far JMP
  through CODE:A2BB. That the selector's base is CODE's is presumed (the
  code there uses CODE's variables), not checked.
- CODE:4DB7: pushed as a return address before a routine that ends in
  `XCHG [ESP],EBX; RET`.
- CODE:BAD6: ten near pointers called in turn by the number in CODE:CBCC
  (CODE:B976 onward) while it is below 0Ah; at 0Ah it goes to CODE:BA7D,
  `MOV AX,0; INT 93h` (presumably pMAX's exit, not checked) followed by
  data, hence a `stop` hint (new in doskit for this).
- tables of near pointers: CODE:910E (4), CODE:268FE (4), CODE:26957
  and CODE:26967 (4 each, by the byte at CODE:A311 less one),
  CODE:27723 (6), CODE:30D78 (2; its index range not checked); the far
  jump back to CS at CODE:A4A3 (to CODE:A4A9).
- 20 computed jumps: a dword constant less the byte sum of a stretch of
  the image (CODE:4CE8, 4CF2, 7438 and one inline at CODE:4D90),
  presumably against a changed program. The targets were worked out
  from the file's bytes (build/cksum.py, a throwaway script, not kept);
  all 20 are at a routine's first instruction after a RET or a gap's
  start, four more were reached already. CODE:7AB6 sums over the far
  pointers start-up writes and gives no address from the file: left out.

- eight compiled switches, word tables of offsets from the table
  (`MOV SI,[EDI*2+T]; LEA EBP,[EDI+T]; JMP EBP`; `rwords`, new in
  doskit): CODE:1183F, 11AE9, 13784, 2C3DD, 2C8BC, 2F91C, 300B9,
  308C7; the index mostly the previous entry kept in a dword at
  CODE:0020..0028. Counts from a bound check or up to the first target
  (CODE:11AE9: a first try with 80h entries ran into the 78h data words
  after the eight, whose targets fell inside instructions).
- CODE:14DE2: `MOV EBX,14DE2h; CALL EBX` (CODE:298CA).
- the two tables of 11 far pointers at CODE:618D and CODE:98FE (ended
  by FFFFFFFFh), handed to the sound driver in EDI before its calls at
  CODE:71E9 and CODE:9B24: presumably its host callbacks
  (docs/audio-driver.md), not checked. The kit does not take far
  pointers for code by itself (CODE holds data too), so 22 `code` lines.
- CODE:29912: four pointers CODE:298A7 calls in turn through the
  pointer at CODE:2982E (29912h in the file). CODE:29873 sets it to
  29922h, a like table after it (and CODE:29932 looks like a third);
  CODE:29873 is not reached, so neither table is in the hints.
- CODE:7ABC..80BC: 192 records of 8 bytes, an ascending dword and a
  handler (14 different ones); CODE:704E calls [ESI+4] while the counter
  at CODE:8124 has reached the first dword. The intro's timing,
  presumably (not checked). `words ... stride=8`, new in doskit.
- two interpreters of word streams (CODE:2CF36 and CODE:2F4A4, a second
  caller each): a word of the stream indexes records of 4 bytes, a
  16-bit offset from the table's second record and a step for the
  stream; 0 ends the stream. CODE:2D18F (32 records) and CODE:2F625
  (26, record 0 at CODE:2F621 is 0,0). No bound check: counts up to the
  code after the table. `rwords ... stride=4 from=`, new in doskit.
  docs/bpc-module.md describes the first as the event interpreter.
- CODE:2DA0A: 28 pointers, `PUSH 2DA09h; PUSH [EBX*4+2DA0A]; RET`.
- CODE:2CD10: 11 pointers the program puts in CODE:0010 before it calls
  into the table module (docs/bpc-module.md: the host vector); the
  entries are instruction starts, what each does not looked at.

Calls that leave the image: through the sound driver's entry (far
pointers at CODE:8134, CODE:98A0); through the fields of the state
record at [CODE:0014] (CB3Eh or 12844h; `[ESI+2946h]`, `[ESI+294Ah]`,
`[EDI+2926h]`, `[[ESI+2A70h]+4]`), zero in the file, which
docs/bpc-module.md has as the table module's header copied to F3E4h;
`CALL [EAX]` at [CODE:A11D]+9Ch, the module's base (the same notes).
Not checked in a run.

### The gaps

Looked at one by one on 2026-09-29 (a throwaway classifier: zeros,
text, dwords into the image, a clean decode; then by eye). Most are
data: inline strings (the `INT 94h AH=1` file names around CODE:75C1..
785B, messages), zero-filled buffers (CODE:6348, 32DE3, 33068 and on),
tables (the scan-code table at CODE:26A6E, the records at CODE:168C9..
2600D, 63,300 bytes, 54h apart in part), and CODE:3D409 to the end
(36,983 bytes) not looked at closely. CODE:4A1B..4CB6 looks random; its
address is stored as data at CODE:4552 (`MOV [3B54h],4A1Bh`, read at
CODE:3FCA as EBX): a table, presumably (not checked).

Code with no reference found (no dword, no immediate, no relative
jump or call to its start anywhere in the image), left as data:

- eight 12-byte pieces `MOV AL,40h; MOV [5DCBh],EAX; JMP 5140h` right
  after routines (CODE:2396, 2541, 254E, 26F1, 277D, 28C3, 29F3, 2CB5;
  two of them are the start of a checksummed stretch), and CODE:5140
  itself (a loop on `SUB EAX,[EBX+3]` with EBX = [5DCBh], then on
  into CODE:5198); near the text "Manual Protection" (CODE:5DE3):
  part of the copy protection, presumably. In a run of the GOG release
  the protection's question did not come (the user's test, reported
  2026-09-29; how often or when the original asks is not known here).
  Fits: the question's texts at CODE:5DE3, 5DF5 ("Please enter word"),
  5E12 (" at page") have no reference anywhere in the image, not even as
  a 16-bit value. How it was switched off (code cut loose, a flag) is
  not found; so these pieces are presumably dead in this release;
- routines after a RET: CODE:2FFA, 36A1 (waits for scan code 2 and its
  release), 73D8, 92CE, 9606 (after the text "HEJ!$"), A618/A61E,
  15340, 297A6, 2E886 and 302F8 (both in the interpreters' style),
  35BC6, 35D20, 29832 (the one that sets CODE:2982E to 29922h);
- one to three bytes after a RET or JMP (CODE:A497, A653, 32902,
  32A79, 3300F, 35E37).

Labels that are not addresses: the kit took a displacement from 100h
up with a register for an address (METHOD.md), but here records have
fields beyond 2900h (`MOV EDI,[ESI+28AAh]` made a label C28AA inside
code), and buffers are written at offsets like `[EDI+49Ah]`. Since
doskit 0cf3171 (2026-09-29) a displacement with a register that lands
in reached code, at an instruction or inside one, is written as a
number: 113 labels fewer (the 82 inside instructions among them), still
IDENTICAL, the gaps unchanged. Those that land in data are still labels
and may be field offsets too (not looked at); a `num` hint for each.

Seen on the way (in doskit's commits): the image's segment-register
stores to memory carry 66h (`66 8C ...`, four of them), which the source
now writes as `MOV WORD PTR [..],DS`; a pointer variable at CODE:8128
is read and then addresses memory (`MOV ESI,[8128h]`), and the kit had
taken the constant stored into it (CODE:7ABC) for code: about 2000
"instructions" of data, now a data label. The earlier scanner reached
12,018 bytes; not compared (the kit reached 11,405 then, 68,151 now).

Not done: everything the method's stage 1 asks beyond the byte identity
(the `ptr`/`dptr` of the many 32-bit immediates that
are offsets, which the analysis does not find by itself; names).

## The loader in the runner

Since doskit f6876cf (2026-09-29) the runner emulates the 386's protected
and V86 mode (no paging, no task switches; doskit's tests/pmode checks
it). Before, the loader was lost at once (its `INT 15h AX=8900h`, the
BIOS's switch to protected mode, failed and it went on in real mode as
if in protected mode).

`python3 doskit/tools/run.py -until 30 ILLUSION.EXE ILLUSION.CFG` now
(runs of 2026-09-29, the doskit commits up to 0d71ad0):

- the banners, then "No memory manager present", "00562kb low and
  15296kb high memory available" (the runner's INT 15h AH=88h says
  15360 KB; the 65472 KB of the earlier run came from elsewhere, not
  looked at);
- the loader enters protected mode through INT 15h AH=89h (the trace
  shows CS 30h, DS 18h, ES 20h, SS 28h after it). For a DOS call it goes
  back to real mode by itself (MOV CR0; e.g. 0030:1221 -> 0076:1226) and
  calls INT 21h at 0076:123A, the file name copied to 0B3E:0000;
- it opens `C:\ILLUSION.EXE` (the path from the environment) with
  AX=3D02h (read/write), then `cdadd.000` and `illusion.386` as loose
  files (neither is there), then reads its archive; a loose file
  presumably takes the place of the archive's entry (not checked);
- ILLUSION.386 runs (its code is CS 14h: CODE:02A3 = `ENTRY`, as in the
  hints). It takes the configuration file's name from the command line:
  INT 93h AH=11h (presumably the command tail) at CODE:02C7, the first
  word to CODE:086C, INT 94h AH=8 at CODE:02FF. Without an argument it
  prints "Error while initializing configuration file..." and exits
  with 36. The GOG release starts it as `D:\ILLUSION.EXE C:\ILLUSION.CFG /`
  (see "The GOG release on the Mac" below);
- with `ILLUSION.CFG` it creates that file (544 bytes, in
  `build/run/state`), opens `SETSOUND\SETSOUND.DAT` and each `.SDR` as a
  loose file first (none there) and shows the "FLD Sound Driver Setup
  Utility V2.01 (c) 1995 FrontLine Design": the sound card list; Sound
  Blaster 16 (seven times down, Enter), base port 220h (Enter), no IRQ
  or DMA question, "Sound quality?" with LOW recommended (the runner's
  6 M instructions a second, presumably), Enter;
- then the game asks MSCDEX (INT 2Fh AX=1500h at CODE:35B5A, AX=1510h
  at CODE:35B34, both reflected to real mode by pMAX). Without MSCDEX
  (the runner before doskit 6a34e22) it prints "CD error! Please check
  your CD and your CD player." and exits with 255;
- with MSCDEX (doskit 6a34e22, 3f69eda; a run with the configuration
  file already in the layer, so no set-up): at t=2.65 IOCTL output 01h,
  IOCTL input 0Ah (audio disk info), 0Bh (track info) for every track
  from the first to the last, IOCTL output 01h twice; nothing is played
  up to t=150 with the cue sheet's 51 tracks (`-cue`) either. Then
  `SB16.SDR`, `intro\MOD.INT` (the intro's music is a module), the
  chooser's files at t=6.6..7.6 (`cube.rix`, `tube.rix`, `torus.rix`,
  `tinyfont.fnt`, `infodata.mgl`, `menuchar.rix`), the intro's
  (`introani.roy`, `intropix.mgl`, `SCROLL.DLT`, `BKGR.FLD`,
  `PCSKY.FLD`); the intro's pictures to t=90 (text over clouds, the
  credits);
- from about t=100 to 150 (the end of the run) the screen is one still
  picture, black, 304x50 as the runner reads the CRTC. The busiest loops
  are CODE:25C0 (waits for the dword at CODE:1548 to change; written by
  CODE:4C8C every 0.03 s from t=97.8, so it is not stuck there) and
  CODE:6E2A (waits for the vertical retrace). What the game waits for,
  or whether the runner's VGA reads the mode wrongly, is not found. The
  image's base in that run is linear 100F30h (found by its bytes in a
  memory dump; `-watch 102478` is CODE:1548). The runner's `-prof`
  prints linear addresses as if real mode (0014:25CC as 0270C).

The key script for the set-up: `3 down` ... (7 downs 0.3 s apart), `5.5 enter`,
`7 enter`, `9 enter`, `11 enter`, `13 enter`, `15 enter`.

Found on the way and fixed in doskit (each with a check in PMODE.EXE):
an open for writing failed when `build/run/state` did not exist yet
(e447687); a REPNE SCASB with ECX = -1 moved the emulated clock by 4
billion instructions (0d71ad0); setjmp's signal mask made runs under PE
45 times slower on macOS (4a9ac3a); INT 21h AH=57h (a file's date and
time, the set-up sets it) was missing (6a34e22).

### The GOG release on the Mac

Looked at 2026-09-29 in `/Applications/Pinball Gold Illusions.app`
(a Boxer bundle; `Contents/Resources/game/Pinball Gold
Illusions.app/Contents/Resources/Illusions.boxer`):

- `C.harddisk/illusion/Illusions/` holds `game.gog` (57,200,640 bytes,
  the data track), `game.inst` (the cue sheet: track 1 `game.gog`
  MODE2/2352, tracks 2..51 `MUSIC\TrackNN.ogg`, typed MP3 in the sheet,
  Ogg Vorbis in fact), `ILLUSION.CFG` (the SHA-256 in
  docs/reverse-engineering.md) and `ILLUSION.BAT`:
  `@D:\ILLUSION.EXE C:\ILLUSION.CFG /%1 %2 %3 %4 %5`;
- `MUSIC\` there holds only `Track02.ogg`; tracks 03..51 are in
  `game.cdmedia/` beside `C.harddisk`;
- `ILLUSIONS.BAT` (the play) mounts `game.gog` alone as D:, so the Mac
  release plays with no audio tracks; `ILLUSIONS_SETUP.BAT` mounts
  `game.inst` and runs `ILLUSION.BAT /o` (the set-up, presumably; `/o`
  not looked at). A second `game.gog` (8,805,888 bytes, another SHA-256)
  is in `C.harddisk/illusion/`, not looked at.

For the runner the Windows layout was made from symbolic links in a
scratch folder (`game.inst`, `game.gog`, `MUSIC/Track02..51.ogg`); `-cd
-cue FOLDER/game.inst` then prints 51 tracks, lead-out 64:13:38, track 2
at 05:26:20 for 13,717 frames (182.9 s, as its Ogg says).

## Next

1. 32-bit support in doskit, in steps:
   - done 2026-09-29: tasm.py assembles USE32 segments (x86enc32.py;
     doskit selftest step 0, tests/enc32). Every jump shortest, no NOP
     padding, as ILLUSION.386's reachable code shows (all 62 near Jcc
     and 6 near JMP out of short reach; no NOPs). Seen there, the
     encoder follows it: prefixes in the order REP, 66h, segment, 67h;
     `MOV DS,AX` without 66h, `MOV AX,DS` with it; ALU reg,reg in the
     `reg, r/m` form; 16-bit addresses (67h) occur. One `64 66` order
     and one sreg store without 66h are exceptions (raw hints later).
   - done 2026-09-29: ALU EAX,imm8 in the 83h form (the
     accumulator form with imm32 was the kit's mistake, seven places
     here); `stop` hint (disasm.py) for a call that does not return.
   - done 2026-09-29: the pMAX image as a program for disasm.py and
     build.py (`pmax` hint, descriptors as segments, selector
     relocations as SEG fixups, write_pmax), tests/flat in doskit's
     selftest, gaps.py and ptrscan.py for 32-bit programs. What
     write_pmax cannot know it writes as ILLUSION.386 has it: the
     header's first word 0, format 1, allocation = image size, the
     entry as an image offset (descriptor 0 has base 0, so either way);
   - done 2026-09-29: the runner emulates the 386's protected mode
     (cpu.c, bios.c), not pMAX's services, so that other DOS extenders
     run on it too; the game gets to its CD check (see "The loader in
     the runner").
   - done 2026-09-29: MSCDEX in the runner (doskit 6a34e22: D: as a
     CD of one data track; 3f69eda: `-cue`, the tracks of a cue sheet;
     plays are kept on the emulated clock, nothing sounds), INT 21h
     AH=57h. The game gets through its intro (see "The loader in the
     runner").
   - next: the still picture at t=100 (what the game waits for there,
     or what the runner's VGA shows wrongly); then on to the chooser
     and a table, with `-cue`, to see when the game plays CD tracks and
     what else the runtime lacks (x87 is not emulated either).
2. Stage 1 for the main program, on from the above: the gaps are looked
   at (see "The gaps"; the unreferenced code there wants a second look
   once more is known, the copy protection first); displacements with
   a register that land in data (field offsets or addresses, by eye);
   offsets among the 32-bit immediates
   (ptrscan.py); names (doskit/docs/METHOD.md).

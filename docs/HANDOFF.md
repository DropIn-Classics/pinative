# Handoff

State of 2026-09-29: the repository was made from doskit's template on
top of an earlier analysis (docs/*.md other than this file). The game's
files are listed. The main program, `ILLUSION.386`, is in stage 1:
`src/ILLUSION.hints` rebuilds it byte for byte (doskit reads pMAX images
since 2026-09-29), with few hints yet (see "Stage 1" below).
The runner runs the game now (protected mode, MSCDEX, VGA mode 0Dh in
doskit, 2026-09-29) through the intro and the chooser to the first
table, which scrolls by itself (see "The loader in the runner"); a ball
is played on it with the game's own keys (see "The keys", 2026-09-29).
All four tables load and scroll; which CD track each plays is found
(see "The tables and their CD tracks"); a mode's track played in a run
(see "A mode's track in a run"). The two stream languages are read, and
tools/event_streams.py lists every stream of a module (see "The event
language"); what the 28 handlers of a take do is read (see "The slot-15
handlers"). The offsets among the 32-bit immediates have hints (see
"Offsets among the immediates"). Table 2's music chooser seen in a run,
and the game's frame rate found (see "Table 2's chooser in a run");
the command-line options and Esc are found (see "The self-patched call
in a run", the end, and "Esc"); a jingle requested in a run (see "A
jingle in a run"); what each option does in a game (see "The options").

## The earlier analysis

Kept: its notes (docs/*.md) and its data-format tools with their
synthetic tests, `tools/bpc_inspect.py`, `asset_inspect.py`,
`sdr_inspect.py`, `cfg_inspect.py` (`python3 -m unittest discover -s
tests`: 45 tests ok, Python 3.9; 52 with tests/test_event_streams.py,
Python 3.12 on Windows, 2026-09-29). Run on `build/files/` 2026-09-29 they
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
- On Windows (2026-09-29, the GOG install in `C:\GOG Games\Pinball
  Illusions`): `isox.py` on its `game.gog`, `extract --all` in 91 s, the
  four SHA-256 of docs/reverse-engineering.md all matched; the runner
  builds with MSVC (`doskit/tools/run/build.bat`), check.py `all ok` in
  25 s. A run goes about three emulated seconds a second (t=400 in
  125 s). The configuration file and the cue sheet straight from the
  install (in Git Bash with `MSYS_NO_PATHCONV=1`, see "The self-patched
  call in a run"): `-put '\ILLUSION.CFG' "C:\GOG Games\Pinball
  Illusions\ILLUSION.CFG" -cue "C:\GOG Games\Pinball
  Illusions\game.inst"` (its `MUSIC\` holds all 50 audio tracks).

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

`python3 doskit/tools/build.py src/ILLUSION.hints`: IDENTICAL, 30,153
instructions, 4195 labels, 17 lines as DB, 26 s on Windows (2026-09-29,
after the offsets among the immediates, see "Offsets among the
immediates"; 30,022 and 3916 before); gaps.py: 209 gaps, 164,557 of
0x46480 bytes not reached as code (see "The gaps" below). The hints so far: the two descriptors as segments (`CODE` the
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

### Offsets among the immediates

Done 2026-09-29, on Windows: `ptr`/`dptr` hints for 425 of the 32-bit
immediates, a block at the end of src/ILLUSION.hints. ptrscan.py found 49
(a MOV r32,imm whose register then addresses memory); a throwaway scan
(not kept) took every 4-byte immediate from 100h to the image's end that
was no label, 642 besides the branches, and sorted them by how the
register is used next (into an INT, a call, a memory operand, a store,
a compare, a subtraction), then by eye, the unclear ones in the code. As
read:

- pMAX's services: INT 92h takes ESI, a name for the block (texts like
  "Used by ..."; two point at a single 0 byte before a routine, CODE:2F13
  and 39C7), and EBX, its size; INT 94h AH=1 EDX the file name and ESI
  the byte before it; INT 93h AH=4 BL=1 ES:EDX an interrupt handler.
- The selectors the program keeps: CODE:A10B (the start-up alias), 131D
  and 635E hold copies of DS, taken for CODE (presumably; a run checked
  only CODE:A10B, see "The keys"); CODE:130F, A117, 634A/634C and the ones
  from INT 93h AH=5 are other memory (video among them), so offsets used
  with them stay numbers; CODE:3B4B is freed with INT 92h AH=5, where it
  is set is not found: 430Ah and 46AFh (the text "The GREETINGS Page",
  in CODE) stored to CODE:3B58 and read with it are left numbers.
- Numbers kept: counts, sizes, strides (C4E0h, 1500h, 5C0h, 580h), the
  lengths of the checksummed stretches (the kit has no label
  differences), a CD time (32200h at CODE:35DE8).
- Code reached by the new `ptr`s (131 instructions): a second keyboard
  handler at CODE:340A (installed at CODE:33D0; a 16-byte ring of scan
  codes at CODE:1906); the routines handed to the sound driver, commands
  0Eh, 0Fh (with a rate in ECX) and 11h (docs/audio-driver.md):
  CODE:4C85/4C93, and CODE:B99C, B9B9, 9CD4. CODE:B99C (DRV_TICK) counts
  CODE:BAD4, the word display opcode 1Ah blinks by and handler 1Ah draws
  from; how often it runs is not checked. CODE:B9B9 sets the CRT start
  address and the pel panning, the screen's scroll presumably.
- CODE:523B writes EBX to [290FBh+7CBh] = CODE:298C6, the immediate of
  `MOV EBX,14DE2h; CALL EBX` at CODE:298C5; EBX is [[CODE:5DCB]+13h] +
  [CODE:55E3], and CODE:5DCB is what the dead copy-protection pieces
  write (see "The gaps"). It does so only when the byte at CODE:037C is
  1: the 0 before the text CODE:0372 prints, named by no other
  instruction in the image. See "The self-patched call in a run".

After it, ptrscan.py lists two candidates, both numbers by the above
(CODE:3D34, a count; CODE:523B). Not looked at: immediates below 100h,
displacements with a register that land in data (see "The gaps"), and
whether a `dptr` to a text is to its start (the scan took the address as
the program has it).

Calls that leave the image: through the sound driver's entry (far
pointers at CODE:8134, CODE:98A0); through the fields of the state
record at [CODE:0014] (CB3Eh or 12844h; `[ESI+2946h]`, `[ESI+294Ah]`,
`[EDI+2926h]`, `[[ESI+2A70h]+4]`), zero in the file, which
docs/bpc-module.md has as the table module's header copied to F3E4h;
`CALL [EAX]` at [CODE:A11D]+9Ch, the module's base (the same notes).
Not checked in a run.

### The self-patched call in a run

Runs of 2026-09-29, on Windows (hints: the comments on CODE:523B, 4FF9,
50A9, 29889, 14DE2; `-log` on each step, `-watch` on CODE:298C6, linear
12A7F6h with the base 100F30h as before):

- the chooser leaves by Esc (`-key 100 esc`): CODE:4FF9 at t=97.6,
  CODE:505C, 50A9, 5198, 51D6 at t=101.11, CODE:5246; CODE:523B not
  reached, the program exits with 32;
- table 1 (`100 space`, `106 enter`, `130 f1`, `131 enter`, Esc at 150
  and 165, to t=185): CODE:51D6 at t=106.56 (the CD stop noted in "The
  loader in the runner" is this routine's CD_STOP), CODE:523B not
  reached; CODE:298C6 written once, by the loader at t=2.28 (the image
  put in place), with 14DE2h; CODE:298C5 and CODE:14DE2 each 4276 times
  from t=124.12 to t=185, about 70 a second. The two Esc presses left
  the table on screen (not followed).

So the patch is dead in this release, presumably, as the protection's
pieces are (CODE:037C stays 0: in the Esc run, `-watch` on it, linear
1012ACh, only the loader's CS 30h wrote it, 0 both times, at t=2.28 and
t=101.12; the table run did not watch it). CODE:4FF9..5288 is the chooser, not only the
program's end: it runs at every choice of a table. Read on the way, not
run for: CODE:29889 is the frame step (the four routines at
[CODE:2982E], a wait for CODE:BAD1 = FFh, which the driver's
command-0Fh routine CODE:B9B9 sets, then 14DE2h and three more);
CODE:14DE2 calls CODE:14DD6 for each ball on the table. CODE:50A9 picks
one of three keyboard tables (QWERTY, QWERTZ, AZERTY) by a country code
from INT 93h AH=15h (presumably the keyboard's; the runs got QWERTY) for
CODE:2B0A2, upper-case.

Seen by chance: Git Bash turns the lone `/` argument into a path
(`MSYS_NO_PATHCONV=1` stops it); the game then showed its options
screen ("Pinball Illusions Options": balls per game, table angle,
scrolling, multiball maximum, tilt sensitivity, resolution) instead of
the intro. Why, found 2026-09-29 (hint `SETUP_ARGS`, CODE:32F39): the
program reads the command tail from its first `/`, at most 20h
characters, and sets a flag for each S (sound set-up), O (options
screen) and R (options back to the standard) in it, case ignored; `?`
prints the help text and exits. "C:/Program Files/Git/" has all three,
so those runs also cleared the options (R) in the state layer. Runs:
`/?` printed the help text ("ILLUSION S - runs the Setsound soundcard
configuration program ...", exit code 32), `/o` alone the options
screen. The texts come from `SETSOUND\SETSOUND.DAT` in English, German
or Swedish by the country code of INT 93h AH=15h ('SV' Swedish, 'GR'
and 'SG' German). The options are the 200h bytes at CODE:009D
(`OPTIONS`); byte +5 is the resolution (`OPT_RESOLUTION`: Enter on it in
a run wrote 1 and showed "SVGA 640x480"), which picks the display mode
of CODE:910E (see "The frame rate"); the other bytes are not matched to
options yet.

### The gaps

Looked at one by one on 2026-09-29 (a throwaway classifier: zeros,
text, dwords into the image, a clean decode; then by eye). Most are
data: inline strings (the `INT 94h AH=1` file names around CODE:75C1..
785B, messages), zero-filled buffers (CODE:6348, 32DE3, 33068 and on),
tables (CODE:26A70, 26AD7, 26B3E: from ASCII 20h on to small numbers,
presumably characters to a font's glyphs, not a scan-code table as noted
here before; the records at CODE:168C9..
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
  35BC6, 35D20 (a seek, command 83h, to a track's start: the request at
  CODE:35EB7; the play is CD_PLAY at CODE:35DDC), 29832 (the one that
  sets CODE:2982E to 29922h);
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

Names: besides the few given with their findings, 22 routines and tables
named 2026-09-29 from what their comments say (a block after
`SETUP_ARGS` in the hints: FRAME_STEP, the stream runners, the opcode
and handler tables, the four display modes, ...); build.py IDENTICAL.
Most routines have no name yet (METHOD.md: a name when what it does is
known).

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
- at about t=95 the game sets BIOS mode 0Dh (16 colours, planar) and
  reprograms the CRTC to 304x224 (CR 1 = 25h, split at line 444). The
  runner knew no mode 0Dh and left the graphics controller in text mode
  at B8000h, so the writes to A0000h were lost and the screen stayed
  black (the "still picture" of earlier runs; `-vgastate`, doskit
  afb3541, showed it; fixed in doskit 55ecb38). Now the chooser's
  attract screens come ("Pinball Illusions" over turning cubes, "Press
  Space Bar for Table Menu / Press ESC to quit");
- `-key 100 space`: the table menu (Law 'n Justice, Babewatch, Extreme
  Sports, The Vikings, Info); `-key 106 enter` takes the first. At t=106.56
  a CD stop, then `ILLUSION.CFG`, `SB16.SDR`, `data\s001\music.mod`,
  `music2.mod`, `special\vm_data.mgl`, the five `data\misc\font*.m`,
  `anims\allanims.mgl`, `source\t001.bpc` (t=114.8) and `.rel`,
  `link1`, `pixels1`, `mask1`, `hide1.m`, `angle1`, the same with 2,
  `stage.m`, `stage.c`, `masks\lights.mgl`, `drops.mgl`, `masks.mgl`,
  `data\misc\flipdat1.m` (t=121.9). At t=124.08 a stop and a play of
  frames 24470..38187 with `-cue`: all of track 2. Which track goes with
  which table or screen is not looked at further;
- from t=120 the table is on screen, 336x350 in unchained 256 colours
  (Mode X like; split at line 317 for the score display, "5 XXX
  50.000.000"), and it scrolls by itself up to t=200 (a new picture in
  every shot 2 s apart): the attract mode, presumably (not checked). No
  instruction the runner does not know (x87 or other) came up to t=200
  (`-v`). The busiest code then is 000C:98BC: selector 0Ch has CODE's
  base too (the keyboard handler runs as 000C:A076, linear 10AFA6h), so
  that is CODE:98BC;
- the image's base in those runs is linear 100F30h (found by its bytes
  in a memory dump; `-watch 102478` is CODE:1548). The runner's `-prof`
  prints linear addresses as if real mode (0014:25CC as 0270C).

The key script for the set-up: `3 down` ... (7 downs 0.3 s apart), `5.5 enter`,
`7 enter`, `9 enter`, `11 enter`, `13 enter`, `15 enter`.

### The keys

Found 2026-09-29 from the program and runs (hints: `KBD_IRQ` and the
comments after it). The user expected the keys usual in the series
(Down held and let go for the plunger, Shift for the flippers, perhaps
Alt and Ctrl, Space to tilt); that is no evidence for this game
(PROVENANCE.md), it said what to look for. What the program does:

- `KBD_IRQ` (CODE:A076, installed at CODE:A03B through INT 93h AH=4
  BL=1) keeps `KEY_DOWN` (CODE:D984, 256 bytes: FFh while down, the scan
  code, +80h after E0h) and `KEY_TOGGLE` (CODE:DA84). The game reads
  them through the record at [CODE:0014] (CB3Eh in the runs, where
  `KEY_DOWN` is its field E46h), so no address in the code names them:
  doskit's `-rwatch` (699dd31, new for this) showed the readers;
- flippers: left = Left Shift or Left Ctrl, right = Right Shift or
  Right Ctrl (CODE:14757). Run: Left Shift held, the left flipper up;
- Space, Left Alt, Right Alt (CODE:144BA, 145A0, 1461C): a value pushed
  up / left / right and back, one push a press: nudges, presumably
  (the picture not looked at for it; whether too many tilt, not found);
- attract mode: F1..F8 or keypad Enter (CODE:2A7FD); F1 started a game,
  "PLAYERS 1 BALL 1" (the others not tried; presumably players 1..8);
  Esc to CODE:2A8AB (not followed);
- the ball: Enter (main keyboard) launches it with a fixed 1770h
  (CODE:2F300); the ball waits in the lane at the right until then. Down
  (E0 50h or 50h) is read by no instruction from the attract mode through
  two balls (it was held 1.5 and 3 s); no one read `LAST_KEY` then either;
- after the start also read: M (32h), P (19h) at CODE:2B4F2, 2B50F
  (not followed).

A game: `100 space`, `106 enter` (Law 'n Justice), `130 f1`, `131
enter`: the ball moves from t=132, is lost at t=150 with 50.000 points
(no flipper pressed), "PLAYER 1 BALL 2" at t=154, waits for Enter.
`-rwatch` and the other address options take linear addresses for this
image: CODE:X is 100F30h+X in these runs (run.py translates only 16-bit
`SEG:OFF`).

Found on the way and fixed in doskit (each with a check in PMODE.EXE):
an open for writing failed when `build/run/state` did not exist yet
(e447687); a REPNE SCASB with ECX = -1 moved the emulated clock by 4
billion instructions (0d71ad0); setjmp's signal mask made runs under PE
45 times slower on macOS (4a9ac3a); INT 21h AH=57h (a file's date and
time, the set-up sets it) was missing (6a34e22).

### Esc

Found 2026-09-29, on Windows (hints `GAME_PHASE`, `QUIT_TABLE` and the
comments on CODE:2A8AB, 2B63B, 2BAB5). A run on table 1 (keys `100
space`, `106 enter`, `124 esc`, `127 space`, `130 f1`, `131 enter`, `140
esc`, `144 space`, `150 esc`, `154 y`; `-watch` on QUIT_TABLE, linear
10DAFBh; a shot a second):

- Esc in the attract mode (t=124): "REALLY QUIT TABLE?" on the score
  display, the table goes on moving; Space (t=127) back to the attract
  mode. By the code, Y (scan code 15h, KEY_DOWN+15h) says yes; not tried.
- Esc during play (t=140): QUIT_TABLE FFh at once (CODE:2BABB), no
  question; the screen dark at t=141, the chooser's table menu after
  Space (t=144); Esc there (t=150) ends the program ("Thank you for
  playing Pinball Illusions CD.", exit code 32), as in the Esc run of
  "The self-patched call in a run".
- by the code only: the same question at CODE:2B63B (the routine at
  CODE:2B543, presumably while the ball waits for its launch), and Esc
  in CODE:2B1DC's routine sets GAME_PHASE 1 (CODE:2B345).

`GAME_PHASE` (state+8Eh, CODE:CBCC) is the number that picks one of the
ten routines at CODE:BAD6 each round ("Code reached through pointers":
the ten near pointers). A run of a whole game on table 1 (F1 at 130,
Enter every 15 s from 131, no flipper; `-watch` on linear 10DAFCh)
wrote it 32 times: 1 attract mode; F1: 2 game start and at once 6 (the
ball waits for Enter, "PLAYER 1 BALL n"); 4 play; 7, 1.3 s each,
"DON'T MOVE", four times in each of the first two balls and three in
the third: a lost ball served again while the ball save (event opcode
0Bh) runs, presumably, as CODE:2B946 does it; 5 the ball lost ("NO
BONUS"), then 6; after the third ball 5, 3 ("GAME OVER", t=208.01), 1.
The hint on `GAME_PHASE` has the routine of each. Not seen: 0, 8
(extra ball, by the code), 9 (near the text "TILT", presumably the
tilt). Why the ball save came back so often (each serve starts it
again?) is not looked at. The Y key on a QWERTZ or AZERTY keyboard (scan code 15h is Z
on QWERTZ) not looked at.

### The options

Found 2026-09-29 (hints `OPTIONS` and the block after `OPT_RESOLUTION`):
statically from the options screen (CODE:32C22, 32C7C, the counts at
CODE:32DE3) and `SETSOUND\SETSOUND.DAT`'s texts, the readers from a run
on table 1 (a game of three balls as in "Esc", F1 at 130, Enter at 131,
146, 161, to t=175; `-rwatch 10E8A6 E`, the seven words below) and a
search of build/ILLUSION.ASM for their field offsets. One byte of
`OPTIONS` per line of the screen, in its order; `OPTIONS_APPLY`
(CODE:B2E0, once at a table's loading) turns five of them into words of
the state (CODE:CB3E, state+0E38h..0E44h, reached through [CODE:0014];
0E38h, not 1E38h: a first search for 1E38h found nothing):

| byte | line | values (index: text -> word) | read by |
|---|---|---|---|
| +0 | BALLS PER GAME | 0 THREE 3, 1 FIVE 5 -> `BALLS_PER_GAME` | game start (CODE:2A97C, to state+0D36h), zone type 0 (CODE:2C5EA) |
| +1 | TABLE ANGLE | 0 NORMAL 3, 1 HIGH 4, 2 VERY HIGH 5, 3 VERY LOW 1, 4 LOW 2 -> `SLOPE_Y` | CODE:134FF each frame (23,690 reads in the run), CODE:B7CC |
| +2 | SCROLLING | 0 MEDIUM 3, 1 SMOOTH 5, 2 FAST 1 -> `SCROLL_DIVISOR` | CODE:30203, `IDIV` of a distance (3165 reads) |
| +3 | MULTIBALL MAXIMUM | 0 SIX 6, 1 THREE 3, 2 FOUR 4, 3 FIVE 5 | `MULTIBALL_CAP` (CODE:B0E3) at the table's loading |
| +4 | TILT SENSITIVITY | 0 NORMAL 100, 1 EARTHQUAKE 0 -> `TILT_STEP` | CODE:14C06 on a nudge (no read in the run: no nudge) |
| +5 | RESOLUTION | 0..3 (see "The frame rate") -> `RES_CODE` 1, 2, 3, 0 | event opcode 19h only, which asks for 5 |

Two more words are constants: `SLOPE_X` 0 (added to the vector's first
word where `SLOPE_Y` goes to the second: a pull straight down the table,
presumably) and `SERVE_SECONDS` 10 (times `FRAME_RATE` to state+0D3Eh
while the ball waits for its launch; what counts it down not followed).
Byte +6 holds a video mode number for the SVGA modes, not a line of the
screen (not followed).

What follows from the code, not from runs: `TILT_STEP` is added to
state+2A78h on each new press of Space, Left Alt or Right Alt, which
falls by 1 a frame; at C8h the tilt flag state+2A75h is set (header slot
38, GAME_PHASE 9). With NORMAL the third nudge within 100 frames of the
first tilts; EARTHQUAKE never tilts. `MULTIBALL_CAP` lowers the table's
two numbers at CODE:B14E (by CODE:A311, the table's number presumably; table 1: 6, 4; 2: 0, 0; 3: 6, 0; 4: 4, 6) to
the option's and writes the non-zero ones to word +2 of the records at
the module header's +0ACh and +0B0h: the tables' multiball sizes,
presumably (table 2 none). That `SLOPE_Y` is the table's pull and
`SCROLL_DIVISOR` the scroll's smoothness is read from how they are used,
not seen: a run with another `OPT_ANGLE` (a `-poke` of CODE:009E, or `/o`)
would show it in the ball's speed.

### The tables and their CD tracks

Runs of 2026-09-29 (`-key 100 space`, one `down` a second from 103 per
table after the first, `106 enter`; `-cue` with the 51 tracks): all four
tables load and scroll by themselves, Babewatch, Extreme Sports and The
Vikings as Law 'n Justice does (a picture of each at t=140), with no
instruction the runner does not know. The CD track each plays at about
t=124: 2, 14, 26, 39. Nothing else is played up to t=150 (tables 2..4)
or t=240 (tables 1 and 2, with a game started by F1 at 130).

How (hints: `CD_PLAY`, `MUSIC_TRACK` and the comments near them): the
program's MSCDEX calls all go through `CD_REQUEST` (CODE:35B0D); the play
is `CD_PLAY` (CODE:35DDC, found with `-rwatch` on the track table
CODE:35F1B; at that time the program runs as CS 0Ch, DS 04h).
At a table's start CODE:9BCA takes the record at [CODE:F470] (header
slot 35 of the table module, docs/bpc-module.md): its word +8 is the
track (dumped in the runs: 2, 0Eh, 1Ah, 27h), its byte +0Ah (1 for all
four) goes to CODE:CB97; with its bit 0 `MUSIC_TRACK` counts the
track's length down and plays it again. Run on table 1 to t=330: track 2
again at t=283.37, 159.3 s after the first play, while the track lasts
182.9 s (13,717 frames): the countdown is in game frames at 61 a second
while the runner shows 70 (see "The frame rate"). CODE:4FE8 plays track 51 near the chooser's loading of
`infodata.mgl`; it did not run up to t=126.

Tracks 3..13, 15..25, 27..38 and 40..51 are not heard in the runs.
Who asks for them (found 2026-09-29; hints `MUSIC_REQUEST`,
`MUSIC_NEXT` and the comments near them):

- plays during a game come through `MUSIC_NEXT` (CODE:F5BE, state
  +2A80h), which CODE:9E13 plays through `MUSIC_TRACK` when it is not 0;
  its one writer is `MUSIC_REQUEST` (CODE:2F85C): word +8 of an audio
  record (header slot 34's array) to it, byte +0Ah to CODE:CB97 (state
  +59h, the flags `MUSIC_TRACK` reads);
- `MUSIC_REQUEST` is called by opcode 13h of the event interpreter
  (CODE:2D5E0, a dword operand: the record), by a dispatcher on a
  record's type (CODE:3007A; type 4 is an audio record), and with the
  records of header slots 34, 36, 37, 38 (state +292Eh..293Eh, the
  copied header at 28A6h + 4 x slot). Slot 37's (tracks 8, 18, 34, 48)
  comes the first time the score passes an entry of a list at state
  +C0CFCh (CODE:2ABA4): presumably the high-score music, not reached;
- the records' word +8 (a throwaway script over the four modules):
  tracks 2..8, 10..13 (table 1), 14, 15, 18, 19, 22, 25 (2), 26..38
  (3), 39..50 (4); 9, 16, 17, 20, 21, 23, 24 and 51 in no record (table
  2 copies three records over records 0..2 at run time,
  docs/bpc-module.md; the templates: see "The audio records and table
  2's music chooser"). A scan for the
  word 13h before a relocated pointer to a record finds most of the
  others in each module's event streams (heuristic, the streams not
  parsed);
- run on table 1 (`130 f1`, `131 enter`, both flippers every 0.6 s to
  t=330, a Enter every 20 s; 195.000 points, ball 3 at t=160):
  `MUSIC_REQUEST` ten times (slot 34's record 0 three times, event
  opcode 13h with record 1 three times, record 42 three times (by none of the
  callers logged: through CODE:3007A, presumably), slot 36's once), every time track 0, so no play; each
  request was followed by an IOCTL output 03h (audio channel control,
  CD_VOLUME). The events whose records carry a track were not hit.

Which events play them (found 2026-09-29 from the four modules' data
with a throwaway script over their relocations, and the handlers named in
the hints: opcode 9 at CODE:2DA7B, `EVENT_QUEUE`, CODE:2D9DE, 2DCA1,
2DD8A, 2E5A2; not seen in a run):

- every opcode 13h whose record has a track is in a long event stream
  (17 to 73 commands) that is started by opcode 9 only: the table's
  modes, presumably. Opcode 9 makes it the running mode stream (state
  +0D5Eh) unless one runs already (state +0D4Fh). Near its end each such
  stream has an opcode 13h with a record of track 0 (record 3 in table
  1, 2 in table 2, ...): what that does to the music is not found
  (`MUSIC_REQUEST` then writes 0 to `MUSIC_NEXT`; later found: it goes
  back to the module music, see "The audio records ..."; its switch on word +2,
  CODE:2F91C, not followed);
- the opcode 9 is in a short stream reached in one of two ways, both
  from a slot-15 record (docs/bpc-module.md), whose word +2Ch picks one
  of the 28 handlers at CODE:2DA0A. Handler 6 (and 10h, 12h, which call
  it) counts the slot-16 counter at the record's +34h up for the
  current player; the counter's threshold action reached queues its
  dword +4 (the notes' "presentation pointer": an event stream) through
  `EVENT_QUEUE`. Handler 11h queues the stream at +34h itself. Handlers
  13h and 1Ah have a non-counter at +34h too (not looked at);
- per table (counter or slot-15 record: module offsets; "n: t" = the
  threshold n gives track t):
  - 1: counter 421Eh 1: 4, 3: 13, 4: 10, 5: 11, 6: 6, 7: 5, 8: 3
    (threshold 2 no track); counter 52A4h 2, 5, 9, 14: 7 (two streams);
    counter 6A34h 4: 7; record 439Ah (handler 11h): 12;
  - 2: counter 5B7Eh 6: 22; counter 55E4h 5: 25; record 927Ah: 19;
    track 15 (record 12) has no pointer to it; records 0..2 are copied
    at run time from records 3..5, 6..8 or 9..11 (the pointers at module
    9D9Eh, by a word of the array at 9D88h, presumably by player), so record
    2, which most of table 2's modes end with, is track 14 only after
    the third; tracks 16, 17, 20, 21, 23, 24 are in no record;
  - 3: counter 82CAh 1: 32, 2: 35, 3: 33, 4: 29, 5: 27, 6: 30; counter
    7A7Ch 1..4: 31 and 28 (both in each), 5: 36; records 690Ch: 30,
    8456h: 37, 4C7Ch: 38; record 14 (37) also after a word 10h at
    71FCh (not parsed);
  - 4: counter 5F60h 1: 41, 2: 44, 3: 42, 4: 45, 5: 40, 6: 43; counter
    486Ch 2: 46; counter 6282h 3: 49; records 6456h: 47, 64EAh: 50.
  With the slot-35 and slot-37 tracks that is every track in a record
  but table 2's 15. What a slot-15 record is on the table (a target, a
  lane?) is not found; a run that hits one of them would show a play.

### A mode's track in a run

Found 2026-09-29, on Windows (hints: the comments from CODE:2C1DA to
CODE:2E5B7). Table 1's counter 421Eh is not counted by handler 6: two
slot-15 records point to it, 41E6h with handler 15h (counts it up, as
handler 6 does) and 4362h with handler 16h (queues the stream of the
threshold equal to the current player's word +16h of the counter). The
counter's flags are 7, its thresholds 1..8, each with a stream and a
light (module 95BEh ...).

How the ball gets there: CODE:2C1DA checks each ball's centre against a
list of zones, 14 bytes each (x0, y0, x1, y1 in table coordinates, a
type 0..4, a relocated object pointer; -1 ends the list). Table 1 has
two such lists, header slots 5 (at module 33E5h, 13 zones) and 11
(34B1h, 10 zones): so slots 5 and 11 are not only flipper data, as
docs/bpc-module.md had it; that they are the two levels' zones is a
guess (the ball's dword +64h picks the list). The ones for this mode:

- zone 352Fh (slot 11, 190..210 x 195..215, type 1) -> object 4B66h,
  whose dword +6 is stream 4B8Ch (its commands from 4B90h, the number
  used here before): its opcode 1 lights record 4362h;
- zone 3447h (slot 5, 55..85 x 170..200, type 4) -> object 4F18h (the
  slot-15 layout), whose +14h is stream 4F50h (commands from 4F54h): `17 @4362 78h` (opcode
  17h: past the take unless 4362h is lit) and `5 @4362` (the take).

That record 4362h is the START MODE insert on the table is presumed (the
insert is on the screen near the top left), not checked. Threshold 1's
stream (42DEh) is `9 @7FEA`: mode stream 7FEAh, with `13 @11DB9` (track
4) and near its end `13 @11DA1` (track 0).

Runs (table 1, `130 f1`, `131 enter`, both flippers every 0.6 s, Enter
every 20 s, `-watch` on the counter's word +16h for player 0, linear
2D82D4h with the module's base at CODE:1D3170, linear 2D40A0h, the same
in every run so far):

- blind play to t=240: the word +16h counts 1 at the launch (t=132.29)
  and in bursts to 9 (back to 1 by CODE:2DF20), 33 writes; record 41E6h
  taken 27 times; record 4362h never lit (its byte +1 stays 0), so no
  mode and no play. The pointers to streams 4B8Ch and 4F50h are read by
  no instruction in play (only by the loader's relocation, CODE:B762,
  B789), because the zone reaches their object's start;
- forced: `-poke 12F026#1 2D8403 01` (at the first count-up, CODE:2E0F6,
  record 4362h's bit for player 0) and `-poke 12F026#1 2D74E7 "00 00 00
  00 50 01 3C 02"` (zone 3447h over the whole table): 4362h taken at
  t=132.84 (by CODE:2D939), the display "BLOW ALL BOMBS BEFORE TIMER
  REACHES ZERO" at t=134, a CD stop and a play of frames 44518..49376
  (track 4) at t=139.13, a timer on the display at t=140. So the chain
  threshold -> opcode 9 -> mode stream -> opcode 13h -> play is seen
  once, for threshold 1 of table 1; the other thresholds and tables are
  not run. What the 6.3 s between the take and the play are (the mode
  stream's commands before its opcode 13h) is not looked at (but see
  "The event language": a wait of 7 s).

### The event language

Read 2026-09-29, on Windows, from the handlers (hints: one comment per
handler, CODE:2D36E ... 2DBDF and CODE:2F68D ... 2FBA7; the runners at
CODE:2D080, 2CD3C, 2F35C and the display queue at CODE:2FEDB; the zone
types at CODE:2C40F ... 2C719). No run was made for it.

- Two languages. Event streams: a word, the position word at +2,
  commands from +4, run by the event queue (EVENT_QUEUE, one command a
  frame, presumably: a call a frame not checked) and by the mode stream
  (opcode 9). Display streams: a flags word (bit 0: the background
  stream), two priority bytes, the position word at +4, commands from
  +6, queued by CODE:2FEDB. Each table has a record of 4 bytes an opcode
  (handler offset, command size); a position is a byte offset from the
  first command. The HANDOFF sections above named two streams by their
  first command (4B90h, 4F54h); they are corrected to the streams
  (4B8Ch, 4F50h).
- The event opcodes, in short (the hints have what each does): 1 light,
  2 light for a time, 3 block, 4 an object's state by its number at
  +44h, 5 take (points, handler), 6/7/0Dh/12h/15h/16h the fields of a
  slot-16 counter, 8 eject a hole's ball, 9 start a mode, 0Ah jump, 0Bh
  ball save (presumably), 0Ch unblock, 0Eh unlight, 0Fh/10h start/stop
  a slot-26 BCD counter, 11h queue a display stream, 13h music, 14h
  call module code, 17h jump unless lit, 18h eject a hole's ball at
  another hole, 19h no effect, 1Ah the held ball back to be served, 1Bh
  multiball, 1Ch the mode stream waits (for a record to be taken or a
  time), 1Dh/1Fh loop, 1Eh where a waiting mode goes when the multiball
  ends.
- The display opcodes: 1 and 0Ch animations, 3 and 1Ah text, 6, 8, 9,
  0Eh numbers (0Eh the seconds left of the mode's wait), 7 and 0Ah
  waits, 2, 0Dh, 0Fh clear the buffers, 10h a record through
  CODE:3007A, 13h/14h loop, 18h/19h the DAC colours FCh..FFh; 4, 5,
  0Bh, 11h, 12h, 15h..17h are in no stream the tool finds (4, 11h,
  15h..17h are a RET).
- The zone types (the switch at CODE:2C3DD): 0 and 1 a target (points,
  a record through CODE:3007A at +2, an event stream at +6; type 0
  first queues header slot 27's or 28's stream when state+0D2Fh is set:
  a skill shot, presumably), 2 and 3 put the ball onto the level of
  header slots 6..11 or 0..5 (docs/bpc-module.md corrected), 4 a hole
  that holds the ball (its event stream at +14h). The object of opcode
  8, 18h, 1Ah is such a hole; the second operand of 18h is a hole too,
  not a stream.
- Slot-15 records: the points of a take are the packed-BCD dword at
  +28h (05000000h for record 4362h), the word +2Ch the handler; the
  row in docs/bpc-module.md that had the points at +2Ch is corrected.
- `python3 tools/event_streams.py build/files/SOURCE/T001.BPC
  build/files/SOURCE/T001.REL` lists every stream reached from the roots
  the program is seen to use (zones of slots 5 and 11, slot-14 groups,
  slot-15 +14h, +18h and +34h with handler 11h, slot-16 +48h, +4Ch and
  the thresholds, slots 27 and 28) and from the stream operands, each
  with where it hangs from; `--check` only counts. All four modules
  parse to the end of every stream with relocated pointers and no
  stream in both languages: 79/82/68/73 event and 87/98/89/85 display
  streams for tables 1..4 (79/90/68/81 and 87/99/89/85 since handler
  1Ah's entries are roots too, see "The slot-15 handlers"). Each CD track of the table in "The tables
  and their CD tracks" comes out of an opcode 13h in a stream under the
  same counter or record. Slot 33's four records (docs/bpc-module.md:
  passed to CODE:2FEDB at CODE:2A6CE) look like display streams whose
  opcode 1 runs into the next record; they are left out of the roots.
- Table 1's mode stream 7FEAh (threshold 1 of counter 421Eh), as the
  tool lists it: blocks four records, lights six, queues a display,
  `1C wait 0 7 +4C` (7 s), a display, a take, `13 music @11DB9` (track
  4), `18 eject_at @4F18 @4DB0`, starts BCD counter 8428h, lights 82F0h
  and waits for it up to 60 s; at the end unlights, stops the music
  (track 0) and unblocks. So the 6.3 s between the take and the play in
  the forced run are that 7-s wait, presumably; that they are 6.3 and
  not 7: the wait is 7 x 61 frames at 70 a second, 6.1 s (see "The
  frame rate").

### The slot-15 handlers

Read 2026-09-29, on Windows, from the code (hints: a comment per
handler, CODE:2E2E1 ... 2E7FC, and on CODE:2FBBF, 2FD11, 2BD95, 2A4B1,
2BBC6) and a throwaway script over the four modules' slot-15 records.
No run was made for it; the names (score, bonus, multiplier, extra
ball) are presumed from what the code does with the fields.

- The player record (state+0D76h) holds two 12-digit packed-BCD numbers:
  +8 (high word +0, low dword +4), shown by display opcode 8, the score;
  +10h (high word +8, low dword +0Ch), the bonus: at a lost ball
  (CODE:2BD95) it is added up word +12h times (at least once) and the
  sum goes to +8; CODE:2A4B1 then clears +10h unless byte +11h is set,
  word +12h unless byte +14h is set. Byte +10h counts extra balls
  (CODE:2BBC6, beside the text "EXTRA BALL").
- A take pays two numbers (CODE:2FBBF): the record's +24h/+28h to the
  bonus, +1Ch/+20h to the score. The hints and docs/bpc-module.md had
  +24h/+28h as the score; corrected.
- The handlers by what they do (table counts are list entries of the
  four modules, 1/2/3/4): 1 extra ball and header slot 25's first lamp
  (3/3/2/6); 2 bonus held (table 2 only); 5 bonus multiplier = the word
  +34h (5/5/4/5); 8 multiplier held (table 1 only); 6, 15h count a
  slot-16 counter up (16h runs the threshold equal to its word +16h, 18h
  counts down); 7 pays the counter's value, 0Ah pays it up to n times,
  0Eh pays its step; 0Bh raises the value by the step (capped,
  presumably), 0Fh and 1Bh raise the step, 19h lowers it; 10h = 0Bh, 6,
  7 and 12h = 0Bh, 6; 11h queues a stream; 13h pays a slot-26 BCD
  counter's value; 14h sets the counter's word +26h to seconds; 17h sets
  the waiting mode's timer; 1Ah picks a random award. 3, 4, 9, 0Ch, 0Dh
  and 19h are in no record (4 and 0Ch are a RET).
- Two things that look like slips in the program, as read: handler 17h
  computes the timer plus +34h and stores the smaller of +34h and +36h
  instead (table 3's record 75C4h: 5 s, whatever was left); handler 0Ah
  sets the count to 0 only when it is below the word, and does not
  lower it otherwise. Handler 2 writes FFh to [+34h], which is 2 in its
  one record (the next record's first word): CODE:0002, a scratch dword.
- Handler 1Ah (tables 2 and 4, one record each): a number from the low
  byte of CODE:BAD4, the first of 8 entries whose limit is above it;
  entries with flag bit 0 are given once (bit 1 marks them, the number
  goes on by 5Dh). tools/event_streams.py now takes these entries as
  roots: 8 more event streams in tables 2 and 4 each. Table 2's (looked
  at): takes of records, among them 4072h (handler 6) and 7402h (2,
  once), a light of 72FEh (1, once), and one with an opcode 13h of
  record 39 (track 0); table 4's not looked at. Where CODE:BAD4 is counted is not
  disassembled (see the hint on CODE:2F7DE).

### The audio records and table 2's music chooser

Found 2026-09-29, statically (table 2's module disassembled with a
throwaway capstone script, the main program in the hints); none of it
seen in a run. The details are in docs/bpc-module.md, "Audio-control
records" and "Table 2's music chooser"; in short:

- An audio record drives two players: the CD (word +8, a track) and the
  sound driver's tracker module (words +2, +4, +6). Word +2 is the
  driver's request: positive command 0Ah (temporary playback, a jingle
  presumably; 1, 2, 3, 6, 7 or 8 in the records, only the sign is read,
  see "A jingle in a run"), negative (FFFEh, FFFFh) command 8 (the module music from
  an order index), 0 none; +4 is the order index, +6 the module slot (0
  `music.mod`, 1 `music2.mod`, both loaded when a table starts; table
  4 has one module). With a track, +4/+6 are where the module music
  goes on when the track's time is up (CODE:9CEA).
- The switch at CODE:2F91C has two arms of the same code: FFFEh and
  FFFFh do the same. A negative request first keeps the flags and the
  asked-for track in state+5Ah, +2A82h, which no one found reads.
- So the track-0 records at the end of the modes (table 1's record 3,
  table 2's 2, ...) switch from the mode's CD track back to the module
  music, at order index 1 in tables 1 and 3, not a silence. That is also,
  presumably, why the runs heard only the tables' first tracks: the rest of a game's
  music comes from the module, not the CD (not listened to, the runner
  has no sound).
- Table 2 lets the player choose one of three tunes ("BY THE BEACH",
  "MOONLIGHT PARKING", "ROLL ME ON") with the flippers and Enter: the
  event opcode 14h object at module 9A08h, run by the mode stream 92BEh
  (CD track 19 while it runs), which hole 4C50h's stream starts when
  record 927Ah is lit. The choice, one word per player at module 9D88h
  (indexed by state+0D72h, the current player), picks the template
  copied over records 0..2: at a game's start (slot 40, template 0), at
  each player's ball (slot 41) and when the choice is made.
- Record 12 (track 15) is in no template and has no pointer to it:
  table 2's data does not play track 15.
- The host vector's +1Ch is `EVENT_QUEUE`, not a display queue
  (docs/bpc-module.md corrected); table 1's module queues three event
  streams through it, table 2's one. These roots come from module code,
  so tools/event_streams.py does not list them.

Open from it: table 1's and 4's opcode-14h objects (module 9A46h,
97E5h) not looked at; slot 41's lamps (the list at module 994Ah by the
player's word +12h) not checked. The run of the chooser: next section.

### Table 2's chooser in a run

Run 2026-09-29, on Windows (keys `100 space`, `103 down`, `106 enter`,
`130 f1`, `131 enter`, Right Shift at 150 and 152, Enter at 155; `-cd`,
`-log` on MUSIC_REQUEST, MUSIC_TRACK, CODE:9D3C, 9EBA, 9EDB; a shot a
second). Table 2's module is not where table 1's is: [CODE:A11D] was
1AC750h (linear 2AD680h) in this run, against 1D3170h for table 1, so
a `-poke` into a module takes its base from a dump of CODE:A11D first
(the first try poked table 1's addresses and hit nothing). The two
pokes, at the 700th call of CODE:14DE2 (linear 115D12h, t=134.37):
`2B68FB 01` (record 927Ah lit for player 0) and `2B0B63 "00 00 00 00
50 01 3C 02"` (zone 34E3h, hole 4C50h's, over the whole table). Seen:

- t=134.45: MUSIC_REQUEST (record 16), MUSIC_TRACK with 13h, the CD plays
  track 19; the display "CHOOSE LEFT RIGHT / SELECT WITH RETURN", then
  "BY THE BEACH" (t=141); Right Shift: "MOONLIGHT PARKING" (t=151),
  again: "ROLL ME ON" (t=154);
- t=151.03: CODE:9D3C, track 19's time up (16.6 s after its play, the
  track 19.1 s long): CD volume 0 and the driver's command 8, the module
  music, while the chooser still runs;
- Enter (t=155.03): MUSIC_REQUEST with record 2, MUSIC_TRACK with 0Eh,
  the CD plays track 14: record 2 of the third template, as read from
  the data. Then "GYM MODE ENABLED" on the display (t=156; which stream
  shows it not looked at).

The other requests in the run (t=130.02, the F1 start; 131.36, the
serve) went through CODE:9EDB (a negative word +2: command 8), none
through CODE:9EBA (command 0Ah, the jingle): not seen in that run (see
"A jingle in a run").

Where the jingles come from (2026-09-29, statically: the four modules'
slot-34 records and tools/event_streams.py; not run): the records with
a positive word +2 (the driver's command 0Ah) are many (25/22/23/22 in
tables 1..4); in tables 1..3 most have module slot 1 (`music2.mod`) and
order indices 0, 1, 2, ..., in table 4 slot 0 and indices 2Dh..44h. Few
are played by event opcode 13h (6/2/0/9 commands); most by display
opcode 10h ("play", a record through CODE:3007A): table 1 has 100 such
commands, many as the first command of the display stream at a slot-15
record's +18h, presumably shown when the record is taken. The blind
runs took record 41E6h 27 times and logged no command 0Ah; its +18h
not looked at. A run that forces a take of, say, table 1's record 43D2h
(its +18h stream 445Ch plays record 34) with `-log` on CODE:9EBA would
show one: done, next section.

### A jingle in a run

Run 2026-09-29, on Windows (table 1: `100 space`, `106 enter`, `130
f1`, `131 enter`, no flipper; `-log` on CODE:3007A, MUSIC_REQUEST,
CODE:9EBA, 9EDB; `-wav`; shots at t=135, 136, 138). The module's base
was 1D3170h again ([CODE:A11D], linear 10B04Dh, dumped in a run before).
Record 43D2h is taken by event stream 4B8Ch (zone 352Fh of slot 11,
object 4B66h), and the ball runs through slot 5's zones after the
launch, so the poke put a slot-5 zone there: at the 700th call of
CODE:14DE2 (linear 115D12h, t=134.05), `2D74F5 "00 00 00 00 50 01 3C 02
01 00 D6 7C 1D 00"`, zone 3455h (type 1, object 4950h) made to cover the
whole table with type 1 and object 4B66h (its pointer as the module has
them after relocation: a CODE offset, 1D3170h + 4B66h). Seen:

- t=134.18: CODE:3007A with the record at module 11F15h (DI=5085h, the
  low word; the runner prints 16-bit registers), MUSIC_REQUEST, then
  CODE:9EBA: the driver's command 0Ah. Record 11F15h is slot 34's record
  34: type 4, word +2 = 3, order index 0Eh, module slot 1
  (`music2.mod`), no track. So display opcode 10h -> CODE:3007A ->
  MUSIC_REQUEST -> command 0Ah is seen once;
- the score display: "skillshot" (an animation, t=135), then
  "5.000.000" (t=136), "DON'T MOVE" (t=138), as stream 445Ch has it
  (play, `anim_wait @A5EA`, a number). With header slots 27 and 28
  (the streams zone type 0 queues, both `light_for @43D2`), record 43D2h
  is the skill-shot award, presumably;
- the WAV stayed empty (the 44-byte header only): the runner's Sound
  Blaster got no transfer it plays. A 12-s run with `-v` showed no
  unknown port; the runner knows the DSP's 8-bit commands and says it
  is a DSP 1.05, the driver is `SB16.SDR`; why nothing played is not
  looked at (a doskit question, rule 7). So the jingle is requested,
  not heard.

The records' positive word +2 (a throwaway count over the four modules'
slot-34 arrays, up to the first record whose word +0 is not 4; 44, 47,
41, 41 records): 1, 2, 3, 6, 7, 8 (table 1: 2 x18, 3 x6, 6 x1; table 2:
2 x22; table 3: 2 x23; table 4: 1, 2 x10, 3 x8, 6, 7, 8). MUSIC_REQUEST
stores it to state+2A84h, which CODE:9E83 only tests for zero and sign
and then clears (the stores of 0 after CODE:9E13 and CODE:9EDB are the
only other uses in build/ILLUSION.ASM, besides the FFFEh CODE:2B345
stores, the Esc path that sets GAME_PHASE 1): what the number means, if anything, is not found (the table
modules' code not searched for it). The hint on CODE:9E83 and
docs/bpc-module.md had "2 in the records"; corrected.

### The frame rate

Found 2026-09-29 from the run above (`-watch` on CODE:CB8E, linear
10DABEh) and the code (hints `FRAME_RATE`, `MEASURE_RATE`, CODE:910E).
The waits of both stream languages and the music's countdown are in
frames, multiplied by `FRAME_RATE` (state+50h). The display mode's
routine (by the byte at CODE:00A2, the options screen's
resolution: `OPT_RESOLUTION`, VGA 360x350, SVGA 640x480, SVGA 800x600,
VGA 320x240; the runs' mode is the first, 336x350 of it on the screen) sets it to 46h (70)
or 3Ch; then `MEASURE_RATE` times one vertical retrace with the PIT and
stores 1234DCh / the ticks, at most 3Dh. So on a 70 Hz display the game
counts 61 frames a second and runs 70 of them: a game second lasts
61/70 s. Checked against two runs: table 1's track replayed after 159.3
s of 182.9 (182.9 x 61/70 = 159.4), track 19's return after 16.6 s of
19.1 (16.6). That the cap is meant so (and not, say, a 60 Hz rate plus
a margin) is not known; a port that wants the original's timing counts
61 a second at 70 Hz frames. The four mode routines' other effects
(CODE:D890, D892, 9B8E, B922) are not looked at.

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
   - done 2026-09-29: BIOS modes 0Dh/0Eh in the runner (doskit
     55ecb38); the game gets to the first table (see "The loader in
     the runner").
   - done 2026-09-29: a ball played on the first table (see "The
     keys").
   - done 2026-09-29: the other three tables in the runner and each
     table's first CD track (see "The tables and their CD tracks").
     done 2026-09-29: who asks for the other tracks (see "The tables
     and their CD tracks"); done 2026-09-29: which events play them,
     from the modules' data (same section); not seen in a run: a play
     of a mode's track (hit the objects the counters count);
     x87 is not emulated, not needed up to t=240.
2. Stage 1 for the main program, on from the above: the gaps are looked
   at (see "The gaps"; the unreferenced code there wants a second look
   once more is known, the copy protection first); displacements with
   a register that land in data (field offsets or addresses, by eye);
   offsets among the 32-bit immediates
   (ptrscan.py); names (doskit/docs/METHOD.md).
3. Tasks for the next agent (written 2026-09-29; each larger, in order
   of use; the rules in AGENTS.md hold, findings go here and in the
   hints):
   - done 2026-09-29: a mode's track in a run (see "A mode's track in a
     run"; forced with `-poke`, which doskit had already). Open from it:
     a blind run that lights START MODE through zone 352Fh. Done
     2026-09-29 with the next item: the zone types, the slots 0-11 rows
     of docs/bpc-module.md.
   - done 2026-09-29: the event language (see "The event language";
     tools/event_streams.py with tests/test_event_streams.py). Open from
     it (the frame rate done 2026-09-29, see "The frame rate"): the slot-15 update CODE:2E8CD (timed lights);
     the animations of display opcodes 1 and 0Ch (who plays them and
     clears state+2A50h); opcode 4's objects (CODE:28EA6); the hole
     eject at CODE:30BD6 (the words 4Ch and FFCEh it puts in the hole's
     +4); a port of the two interpreters needs these.
   - done 2026-09-29: the slot-15 handlers (see "The slot-15
     handlers"). Open from it: a run that checks the names (score,
     bonus, multiplier, extra ball: `-watch` on the player record);
     who reads a slot-16 counter's word +26h (handler 14h); table 4's
     random awards. Where CODE:BAD4 is counted: DRV_TICK (see "Offsets
     among the immediates"), how often not checked.
   - done 2026-09-29: table 2's music (see "The audio records and
     table 2's music chooser"); its run done 2026-09-29 (see "Table 2's
     chooser in a run"). Open from it: table 1's and 4's opcode-14h
     objects. Done 2026-09-29: a jingle (driver command 0Ah) requested
     in a run (see "A jingle in a run"). Open from it: why the runner's
     Sound Blaster plays nothing (doskit); what the number in a
     record's positive word +2 means.
   - done 2026-09-29: Stage 1 (item 2), the offsets among the 32-bit
     immediates (see "Offsets among the immediates"). Open from it: the
     selector in CODE:3B4B; names for the routines found in these
     sessions (METHOD.md), keeping build.py IDENTICAL (22 given
     2026-09-29, see "The gaps", the end). Done 2026-09-29:
     the self-patched call at CODE:298C5 (see "The self-patched call in
     a run": not patched in runs). Done 2026-09-29: the options screen's
     argument (see "The self-patched call in a run", the end); what Esc
     does (see "Esc"). Done 2026-09-29: the other option bytes (see "The
options"; open from it: a run with another table angle, the countdown
of `SERVE_SECONDS`, the multiball records). Open: the game
     phases 0, 8, 9 of CODE:BAD6 (the others done, see "Esc"); the SVGA
     modes in the runner (FRAME_RATE then 60, presumably).

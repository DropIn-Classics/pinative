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
jingle in a run"); what each option does in a game (see "The options");
the jingle's effect in the WAV, and the runner's breakpoints no longer
change a run (see "The jingle in the WAV"); a jingle alone, which
stops the module's music for its 1.78 s (see "The jingle alone"); the
CD's audio in a WAV of its own (see "The CD in a WAV"); the CD muted
in a game, and around a jingle whose record asks for it (see "The CD's
volume around a jingle"); bit 1 of an audio record's byte +0Ah, read
by no instruction (see "Bit 1 of a record's byte +0Ah"); the lit
records' timers, the lamps' blinking, a light group's stream and the
lane change read from the code (see "The lit records' timers and the
lamps"); the drop targets (see "The drop targets"); the holes' eject (see
"The holes' eject"); the display's animations (see "The display's
animations"); the timed lamp, a drop-target bank and a hole
seen in a run (see "The lamps, a drop-target bank and a hole in a run");
the player record's names checked in a run (see "The player record
in a run"); the slot-16 counters' timers (see "The slot-16 counters'
timers"); table 4's random awards (see "Table 4's random awards"); the tilt (see
"The tilt in a run"); game phase 0, set by no one (see "Game phase 0"); the
SVGA modes in the runner (see "The SVGA modes in a run"); the table angle
seen in the ball's speed and the ball save (see "The table angle and the
ball save in a run"); the multiball option and a multiball in a run (see
"The multiball in a run"); the work on Linux (see "Start here"); the
selector in CODE:3B4B, the chooser's Info page and a hidden greetings
page, the chooser's keys, captions and backdrop, and how its pages are
left (see "The Info page and the greetings page"); table 1's shooting
game on the display (see "Table 1's shooting game in a run"; its
25-hit extra ball never given, see "Table 1's shooting game when the
lives run out"); table 4's
sea game (see "Table 4's sea game in a run"); table 3 has no
opcode-14h object (see "Table 3's opcode-14h record"); the hole's
stale sound read gives nothing in a run (see "The hole's sound in a
run").

## The earlier analysis

Kept: its notes (docs/*.md) and its data-format tools with their
synthetic tests, `tools/bpc_inspect.py`, `asset_inspect.py`,
`sdr_inspect.py`, `cfg_inspect.py` (`python3 -m unittest discover -s
tests`: 45 tests ok, Python 3.9; 52 with tests/test_event_streams.py,
Python 3.12 on Windows, 2026-09-29; 54 on macOS, Python 3.9, with the
drop-target banks' test, 2026-09-29). Run on `build/files/` 2026-09-29 they
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
- On Linux (2026-09-29, Debian 13, gcc 14.2, Python 3.13 with the
  distribution's `python3-capstone` 5.0.7; the GOG Linux installer's
  `~/GOG Games/Pinball Illusions/data/`): `isox.py` on its `game.gog`,
  `extract --all` in 66 s, the four SHA-256 matched; check.py `all ok`
  in 18 s; runner and port build with `sh`. gcc warns 328 times in the
  runner (323 strict aliasing from `dosrun.h`'s register macros); built
  with `-fno-strict-aliasing` a run to t=160 had the same RAM and VRAM
  hashes, so not a problem for these runs. A run goes about 4.6
  emulated seconds a second (t=160 in 35 s). The cue sheet there is
  `data/game.ins` (not `.inst`), `MUSIC\Track02..51.ogg` beside it:
  `-cue "$G/game.ins"` with `-put '\ILLUSION.CFG' "$G/ILLUSION.CFG"`
  and the arguments `ILLUSION.EXE 'C:\ILLUSION.CFG' /`. Table 1 with
  `-cue`: track 2 played at t=130.08, the table in the screenshots. The
  run of "The table angle and the ball save in a run" (NORMAL, no
  `-cue`) repeated with `-watch` on GAME_PHASE: saved at 140.24,
  143.07, 146.06 and also at 150.01, lost for good at 155.965 (the
  macOS notes: 140.23, 143.05, 146.05, "ran out about 147", 155.95).
  Whether the Mac run had the save at 150 too (launch + 610 frames at
  70 Hz + three pauses of 1.3 s is about 150.9) was not checked there.

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
  (Found later, see "The Info page and the greetings page": set by two
  POPs, INFODATA.MGL's selector; 46AFh is read with CODE's DS and is
  now a label.)
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
  BB0E (after the text "9090", a debug tool moving two words with the
  keys 1..4, see "Game phase 0"), 35BC6, 35D20 (a seek, command 83h, to a track's start: the request at
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
while the ball waits for its launch: the ball save's length, see "The
table angle and the ball save in a run").
Byte +6 holds a video mode number for the SVGA modes, not a line of the
screen (`OPT_SVGA_MODE`, see "The SVGA modes in a run").

What follows from the code, not from runs: `TILT_STEP` is added to
state+2A78h on each new press of Space, Left Alt or Right Alt, which
falls by 1 a frame; at C8h the tilt flag state+2A75h is set (header slot
38, GAME_PHASE 9). With NORMAL the third nudge within 100 frames of the
first tilts; EARTHQUAKE never tilts. `MULTIBALL_CAP` lowers the table's
two numbers at CODE:B14E (by CODE:A311, the table's number presumably; table 1: 6, 4; 2: 0, 0; 3: 6, 0; 4: 4, 6) to
the option's and writes the non-zero ones to word +2 of the records at
the module header's +0ACh and +0B0h: the ball counts of two multiball
commands per table (table 2 none; see "The multiball in a run"). That `SLOPE_Y` is the table's pull and
`SCROLL_DIVISOR` the scroll's smoothness is read from how they are used,
not seen for `SCROLL_DIVISOR`; `SLOPE_Y` seen in the ball's speed and
`SERVE_SECONDS` found to be the ball save (see "The table angle and the
ball save in a run").

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
  1Ah's entries are roots too, see "The slot-15 handlers"; 80/90/68/84
  event streams since the drop-target banks' are, see "The lamps, a
  drop-target bank and a hole in a run"). Each CD track of the table in "The tables
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

Open from it: table 1's opcode-14h object (done 2026-09-29, see
"Table 1's shooting game in a run") and table 4's (done 2026-09-29, see "Table 4's sea game in a run"); slot 41's lamps (the list at module 994Ah by the
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
  is a DSP 1.05, the driver is `SB16.SDR`. Why: the runner had no SB16
  (see "The Sound Blaster in the runner"), fixed in doskit since. So
  the jingle is requested; repeated with the fix, see "The jingle in
  the WAV".

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

### The Sound Blaster in the runner

Found 2026-09-29, on macOS (runs of `ILLUSION.EXE ILLUSION.CFG` with the
configuration file of "The loader in the runner": Sound Blaster 16,
220h, quality LOW). The driver `SB16.SDR` sits at linear 1473C0h in
these runs (selector 0Ch; its banner there reads "MS32 Sound Driver for
No sound", the set-up's template text presumably, not checked). With
the runner's ports logged (a local change, not kept):

- t=2.77: it writes 80h and 81h to the SB16 mixer's index port 224h and
  reads 225h after each (driver +A33..+A5E, read from a capstone
  listing of that memory): the lowest set bit of register 80h picks the
  IRQ (through a table at +C9h), the highest of register 81h AND EBh the
  DMA channel (to +8C0h). The runner had no mixer and answered FFh, so
  DMA 7, presumably (from the code, not run that way);
- t=14.02 (the intro's module starts): DMA channel programmed, DSP
  reset, D1h, 41h 3E80h (16000 Hz), B6h 30h (16-bit, auto-init, FIFO,
  stereo, signed) with length 4AFFh. The runner knew neither 41h nor
  Bxh ("unhandled DSP command"), had no mixer and only the first DMA
  controller (channels 0-3): nothing played. The 12-s run above ended
  before t=14, so it saw no port at all.

Fixed in doskit c44cf02: a Sound Blaster 16 (mixer registers 80h-82h:
IRQ 7, DMA 1 and 5; the second DMA controller, channels 4-7; DSP 41h,
42h, Bxh/Cxh, D5h/D6h, D9h/DAh; the 16-bit interrupt acknowledged at
22Fh; `-sb` prints the transfers). Now the driver's transfer is 16-bit
stereo at 16000 Hz on DMA 5, mode 59h, 9600 words at linear 13120h,
the DSP's block 19200 words: the interrupt every 0.6 s, every other
wrap of the DMA buffer (the driver presumably mixes by the DMA
position, not checked). `-wav` has sound from t=14 (a run to t=40: 26 s;
a run to t=140 with `100 space`, `106 enter`: 122 s, a second start at
t=104.19). Looked at as numbers only (loudness and zero crossings a
second: a fade-in over 7 s, about 1400-3400 crossings a second, not
noise); not listened to.

### The jingle in the WAV

Runs of 2026-09-29, on macOS, with doskit c44cf02 and then 69b5070
(the runner's fix below). The run of "A jingle in a run" again: `-cd
-cue` on the Mac layout of "The GOG release on the Mac" (without `-cd`
the table did not come by t=142, "Loading" on the screen), the same
poke, `-log` on CODE:3007A, MUSIC_REQUEST, CODE:9EBA (as linear
addresses, base 100F30h: run.py names only ILLUSION.386's addresses,
not ILLUSION.EXE's), `-wav`; and a control run without the poke.

- The keys moved: the intro now lasts to t=101 (320x290 up to t=100,
  the chooser's 304x224 from t=102; the runs before c44cf02 had mode
  0Dh at t=95), so `100 space` fell into the intro and `106 enter` did
  nothing. That the working Sound Blaster makes the intro longer is
  presumably so, not checked. Used: `106 space`, `112 enter`, `136 f1`,
  `137 enter`. The table's module transfer starts at t=130.10 (16-bit
  stereo 16000 Hz again, but a block of 3840 units, the chooser's
  19200), track 2 at t=130.08.
- As on Windows: the poke at the 700th call of CODE:14DE2 (t=140.05),
  CODE:3007A with DI=5085h at t=140.188, MUSIC_REQUEST, CODE:9EBA
  (command 0Ah) at t=140.188. The control run has no call of
  CODE:3007A or CODE:9EBA up to t=150; both have MUSIC_REQUEST at
  t=136.01 and 138.30 (record words not looked at).
- The WAVs (mono 16000 Hz, the runner's mix) are the same sample for
  sample up to t=140.24 (counted back from the end at t=150), 50 ms
  after the command. Then, loudness per 0.25 s: 4877 against 2143 in
  the control run at t=140.24, 3167 against 1724, then about the same
  level, near silence from t=141.74 to 142.2 (74, 0, 571 against 1432,
  1362, 1667), then music that stays different. A jump of the module
  to order 0Eh, as the record asks, would sound like that, presumably;
  but the poke also changes the play (the award, the ball), so not all
  of the difference is the jingle. Not listened to (in `-wav` of such a
  run from 1:45 on, sample 1680734).

The first comparison differed already at t=136, before the poke: the
runner's `-log`, `-poke` and `-shot` changed the run (at t=139 a plain
run, one with `-log` on MUSIC_REQUEST and one with the poke not yet due
had three memory hashes). A breakpoint ended the runner's batch of
instructions early and a new batch began, so the points where it ticks
the devices and takes interrupts moved; a `-log` in a tight loop kept
interrupts out altogether; shots bounded the batches too. Fixed in
doskit 69b5070 (a test in its selftest): now the four give the same
hash, and the plain run's is the one it had before the fix. So the
runs before this fix that used `-log`, `-poke` or `-shot` may have run
a little otherwise than without them; their findings are what the
code did in those runs, the timings to the millisecond perhaps not
what a run without looking gives.

### The jingle alone

Runs of 2026-09-29, on macOS, doskit 69b5070 (the keys and the Mac
layout of "The jingle in the WAV", `-until 150`, `-wav`; 21 s each). To
hear a jingle without changing the play, the poke wrote the request
itself, record 11F15h's words, where MUSIC_REQUEST puts them: at the
700th call of CODE:14DE2 `CODE:F5C6 "0E 00 01 00"` (order index 0Eh,
module slot 1), at the 701st `CODE:F5C2 "03 00"` (the request, word +2;
in linear addresses, base 100F30h). Not written: MUSIC_NEXT and
CODE:CB97 (the record's track 0 and its byte +0Ah, not looked at).
A control run without the pokes.

- CODE:9EBA (command 0Ah) at t=140.071, once; the control run has none.
  Both runs have the same MUSIC_REQUEST and CODE:9EDB calls at t=136.01
  and 138.30, the same shots at t=139 and 145 (byte for byte) and the
  same VRAM hash at t=150: the play is not changed; the RAM hashes
  differ (the sound's state, presumably, not compared).
- The WAVs are the same up to t=140.119, 48 ms after the command.
  Loudness per 0.25 s from there: 4836, 3355, 1391, ... against 2145,
  1844, 1601, and near silence at t=141.62 (75 against 1785): the
  numbers of the award run of "The jingle in the WAV" (4877, 3167, the
  silence at 141.74), so the difference there was the jingle, not the
  award.
- After it the music is the control run's, 1.78 s late: the loudness
  per 10 ms from t=142 to 150 correlates 0.895 with the control run's
  shifted by 1.78 s, 0.017 unshifted (the same for 142-146 and
  146-150); not sample for sample (no exact match within 4 s). So the
  jingle takes the module's place and the module goes on from where it
  stopped, as docs/audio-driver.md read from the driver (command 0Ah
  keeps the module's position for later).
- The jingle's length from the data: order 0Eh of table 1's
  `MUSIC2.MOD` is pattern 14, speed 4 (F04) for rows 0-5, 1 (F01) for
  row 6, 4 from row 7, and at row 22 B0Eh, a jump to its own order: 24
  + 1 + 64 = 89 ticks, 1.78 s at 50 Hz (the default tempo; the
  pattern sets none). The patterns at orders 0Dh, 0Fh, 10h also end
  with a jump to their own order (B0Dh, B0Fh, B10h), so that jump ends
  a jingle, presumably (read in the driver since, and seen: see "The
  CD's volume around a jingle").

Not listened to. The CD is not in the WAV (the runner keeps its plays
on the clock only), so bit 2 of CODE:CB97 (the CD's volume 0 before a
jingle) is not seen. (Since doskit e2c9cce the runner writes the CD's
audio and prints the channel settings, see "The CD in a WAV"; the
volume around a jingle: "The CD's volume around a jingle".)

### The CD in a WAV

doskit e2c9cce (2026-09-29): `-cdwav FILE` writes what the CD drive
plays, 44.1 kHz stereo from t=0 on the emulated clock, from the cue
sheet's track files (the Ogg Vorbis decoded by stb_vorbis, doskit's
third_party/); IOCTL output 03h (the channels and their volumes) is kept
and printed by `-cd`, IOCTL input 04h reads it back (before: always full
volume). `-wav` (the Sound Blaster) is a file of its own, starting at the
card's first transfer, not mixed with it.

Run on macOS (the Mac layout of "The GOG release on the Mac", `-cd
-cue`, `-put` of the bundle's `ILLUSION.CFG`, `106 space`, `112 enter`,
`-until 150`, `-wav`, `-cdwav`; 19 s):

- a stop at t=112.57, a stop and the play of frames 24470..38187 (track
  2) at t=130.084; the WAV is silent before (0 samples not 0 up to
  sample 5736716, the play's), sound from sample 5737284 (the track's
  own leading silence, 568 samples);
- from the play on, the WAV's samples match libsndfile's decoding of
  `Track02.ogg` (soundfile 0.13.1, outside the kit) within 1, mean
  difference 0.085, no shift;
- no IOCTL output 03h up to t=150: the game did not set the CD's
  volume in that run.

Not listened to. (In that run the game was not started: the CD's
volume 0 comes with a game's start, see "The CD's volume around a
jingle".)

### The CD's volume around a jingle

Runs of 2026-09-29, on macOS (the Mac layout, `-cd -cue`, the bundle's
`ILLUSION.CFG`, `106 space`, `112 enter`, `136 f1`, `137 enter`,
`-cdwav`, `-wav`; logs as linear addresses, base 100F30h, and in
SB16.SDR, linear 1473C0h in these runs as before; doskit cb97a46, then
381ea4f, the fix below). Hints: CD_VOLUME, MUSIC_REQUEST, the comments on CODE:9DC7,
9C1A, 9C4F, 9E83, 9CD4; the driver's side in docs/audio-driver.md.

- A game's start mutes the CD: the negative requests (t=136.01 at F1,
  138.30, 139.01) each set channels 0 and 1 to volume 0 through IOCTL
  output 03h (CD_VOLUME; `-cd` prints `0<-0 00, 1<-1 00`). The table's
  track 2 is not stopped: `-cdwav` has its sound up to t=136.010646 and
  silence after. So in a game the music is the module's, with the CD
  running muted.
- A mode's track (table 1, START MODE forced as in "A mode's track in a
  run", track 4 at t=145.13) comes with CD volume C0h and the driver's
  level 0 (command 0Ch, CODE:9C1A): the module silent under the CD.
- Byte +0Ah of the audio records (a throwaway count over the four
  modules' slot-34 records): 1 in every record with a track (bit 0, the
  timed track), 2 in a few, 4 in 12/20/18/0 of the jingle records of
  tables 1..4, 0 in the others; table 1's record 34 (the jingle of the
  runs before) has 0.
- A jingle whose byte has bit 2, poked while track 4 plays (at the 1500th
  call of CODE:14DE2: CODE:F5C6 `0F 00 01 00`, CODE:CB97 `05`; at the
  1501st CODE:F5C2 `03 00`, record 35's words): CD volume 0 at t=151.434,
  command 0Ah (order 0Fh), the driver's end at t=154.148 (B0Fh), the
  completion routine CODE:9CD4, then CODE:9DC7: CODE:CB97 back to 01 from
  CODE:CB98 and CD volume C0h at t=154.162. The CD WAV is silent from
  t=151.434456 to 154.161746; the track ran on under it (no stop, no
  play), so it comes back 2.73 s further on. A jingle without bit 2 in
  the same run (the game's own, t=147.32, order 2) left the CD at C0h and
  set C0h again at its end (t=149.28).
- How the driver ends a jingle (read in SB16.SDR, seen in these runs):
  a Bxx whose target is the order being played, in temporary playback,
  sets the driver's flags; the next row restores the saved module, and
  command 6 (every frame, CODE:298EF) calls the completion routine. The
  jingle alone again with the driver logged: command 0Ah at t=140.071,
  B0Eh at t=141.763 (1.692 s: row 22's first tick after 24 + 1 + 15 x 4
  = 85 ticks, 1.70 s at 50 Hz), the callback at t=141.764; the module
  comes back at the next row, after 89 ticks (the 1.78 s of "The jingle
  alone").

A runner bug on the way: the first runs with track 4 lost the Sound
Blaster for good at t=145.13. SB16.SDR's command 2 (pause, resume) masks
and unmasks DMA 5 around each CD request (CODE:9C4F, 9C5E); the runner
ended the transfer when it met a masked channel, and the track's start
(a CD stop and play between the two) was long enough for a sample to
come due. The module's sequencer runs in the driver's mixing loop, so it
stopped too (its tick count, driver +31F2h, stood still from t=145.14),
no jingle ended, no completion came, and the CD stayed at volume 0 after
the bit-2 jingle. Fixed in doskit 381ea4f (a masked channel holds the
transfer; SB16.EXE's check 5); run.py learnt `-cdwav` in cb97a46 (it
had taken it for the program). A run since doskit c44cf02 in which a track
started during play had no Sound Blaster from there on. The runs of the
sections above had, after the table's start, only the pairs around a
volume setting (t=136-139), which did not stop it in this run's
configuration (the tick count went on to t=145); presumably not in
theirs either, not checked run by run.

Not listened to. Bit 1 of the byte: see the next section. Open: a bit-2 jingle whose track's
time runs out during it (CODE:9D3C then, not run).

### Bit 1 of a record's byte +0Ah

Runs of 2026-09-29, on macOS (the bundle's `ILLUSION.CFG` by `-put`, no
`-cue`; keys `106 space`, `112 enter`, `136 f1`, Enter at 137 and every
15 s after, no flipper; `-watch` and `-rwatch 10DAC7 1` on state+59h,
CODE:CB97; `-log` on MUSIC_REQUEST, its bit-1 branch CODE:2F888 and
CODE:2B9F6, linear 13078Ch, 1307B8h, 12C926h). Hints: LOST_BALL_RUNOUT,
MUSIC_REQUEST, the comment on CODE:2B9F6.

- Bit 1 is read by no instruction. The readers of state+59h, by the code
  (every `[CCB97]` and `+59H]` of build/ILLUSION.ASM) and in the runs:
  CODE:9D99 (TEST 1, the timed track), 9E9A (TEST 4, the CD's volume
  around a jingle), the OR that sets the bit (2F89A) and the copy to
  +5Ah for a negative request (2F8E3). So a record's byte 2 (the ones
  "in a few" records of "The CD's volume around a jingle") does nothing
  found either; the lost ball's own record in these runs has it
  (state+59h 02 at each loss, by the normal branch).
- CODE:2BAD3 (LOST_BALL_RUNOUT) is set by CODE:2B9F6, a lost ball without
  the ball save: it steps frames there until the two event streams and
  the mode stream are empty, then clears it and sets GAME_PHASE 5. Only
  MUSIC_REQUEST reads it. With no stream running it was set for 14 ms
  (t=155.951..155.965), and the bit-1 branch did not run in a game of
  three balls.
- With START MODE forced (the two pokes of "A mode's track in a run",
  and the zone put back to its own bytes at MUSIC_REQUEST's third call,
  t=139.0, as over the whole table it held the ball) the ball was lost at
  t=148.494 during the mode, and 128 ms later the mode's closing record
  came through the bit-1 branch (t=148.621, state+59h 02, MUSIC_NEXT not
  written, the CD muted); without the loss the same record comes when the
  mode's timer runs out (t=197.3). That the mode stream gets there by
  state+0D51h (CODE:2CD3C) is presumed, not traced.

So a port can leave bit 1 out; what LOST_BALL_RUNOUT changes is only
that MUSIC_NEXT is not written during the run-out. Not listened to;
other tables not run.

### The lit records' timers and the lamps

Read 2026-09-29 from the code (hints: LIT_LIST_STEP CODE:2E8CD,
LAMP_OFF CODE:2E82F, LIGHTS_STEP CODE:2EF7A) and header slot 14 of the
four modules (a throwaway script with tools/event_streams.py's loader).
No run was made for it.

- The lit slot-15 records are on a list (state+2A32h, next +30h) that
  LIT_LIST_STEP walks once a frame. A record not lit for the current
  player leaves it with its lamp off. A timer (opcode 2: seconds x
  FRAME_RATE) is counted down a frame; while more than FRAME_RATE is
  left the lamp is put on blinking each frame, for the last game second
  it is put off each frame, at 0 the record is unlit and leaves. Records
  without a timer (opcode 1) have their lamp put on blinking each frame.
  Not for a record blocked for the player, and not while a mode runs
  for records with flag bit 4 (opcode 9 turns those off).
- So a lit record's lamp blinks. The blink is LIGHTS_STEP's: a light
  state with byte +2 bit 1 counts +3 down per visit and inverts +1 when
  it runs out (+4 = 8 from the lit list). LIGHTS_STEP visits about 20h
  lights and draws at most 8 changes a call, so the blink period in
  frames depends on the table's number of lights: not measured.
- A group of header slot 14 (flags byte +4 bit 1 clear: the groups with
  an event stream) whose lights are all on and none blinking queues its
  stream once (bit 0 of +4 marks it) and goes to a list at
  state+1822h (not followed). So a group's stream comes when all its
  lamps have been taken (a lit record's lamp blinks until taken; a take
  sets the player's bit in byte +5 of the record's other light state,
  +8, see opcode 5), presumably; not seen in a run.
- On a press of either flipper key the groups of a second list, stored
  backwards in front of slot 14's pointers, have their lamps' on bits
  moved one light along the chain (the last gets the first's): the lane
  change, presumably. One group in tables 2, 3 and 4 (3, 3, 4 lights),
  none in table 1.
- Open (the dark last second and the blink seen, see "The lamps, a
  drop-target bank and a hole in a run"): what the list at state+1822h is for; who clears a group's bit
  0 (CODE:2EEEC clears the lights of a chain, not followed).

### The drop targets

Read 2026-09-29 from the code (hints: DROP_SET, DROPS_DRAW, DROP_MASK,
OBJECT_HITS, DROP_HIT, DROPS_RAISE_STEP, DROPS_QUEUE_STEP,
DROPS_UP_ALL, DROPS_UP_BALL; the fields in the comment above them) and
the four modules (a throwaway script). No run was made for it.

- Event opcode 4's objects are drop targets: objects of type 1 in the
  table of 0C0h dwords at header slot 4. Table 1 has one bank of 2,
  table 4 three banks (3, 3, 2), tables 2 and 3 none (read as the code
  reads the table; entries past its real end may be other data, table
  3 shows one with a type 46AEh).
- A target's state is drawn by DROPS_DRAW each frame when it changed:
  a picture from drops.mgl, and its mask from masks.mgl cleared from
  (down) or set in (up) a 1-bit map of the level, 42 bytes (336
  pixels) a line; that the ball collides with that map is presumed.
- A hit (OBJECT_HITS, by the ball's word +6Ch; who sets it is not
  followed): the target's lamp bit, the bank's points, a sound record,
  the target down. When the whole bank is down: the bank's event
  stream, and unless the bank's +4 bit 0 is set, the bank is raised
  64h frames later (one target a frame through the queue at
  state+2A60h). All targets go up at the game start; at the next ball
  all but those with +4 bit 0.
- Opcode 4 sets the state directly (0 up, 1 or FFFFh down) and clears
  +0Bh, so a target it puts down does not count as down for its bank.
- Open: the object types 0 and 2 of OBJECT_HITS; the table CODE:285BC
  that CODE:28E7B fills and CODE:28F41 draws (another kind of object,
  not followed); a bank that is raised again (table 4).

### The holes' eject

Read 2026-09-29 from the code (hints: HOLE_EJECT_STEP CODE:30996,
CODE:30BD6, CODE:30CFB). No run was made for it.

- Opcodes 8 and 18h push a hole on the stack state+2A64h; one hole at a
  time is ejected (state+2A68h). The words HANDOFF asked about are its
  count +4: 4Ch (76 frames) when the hole the ball comes out of has an
  object at +30h (a picture of the table CODE:285BC), else FFCEh (-50).
- With a picture: 16 frames wait, the ball out at count 3Ch, then for
  the last 50 frames the picture flickers (count AND 4) and every 8th
  frame the hole's sound record +10h; at 0 the hole is free. Without:
  the ball out after 50 frames, nothing drawn.
- The ball out: its place from the hole's +6, +8, its speed words get
  the hole's +0Ah, +0Ch added (their high bytes cleared first), and it
  goes onto the level +0Eh as zone type 3 (0) or 2 (1) does. With
  opcode 18h all of that is the second hole's.
- A slip, as read: for a hole ejecting its own ball the sound record
  is read from a scratch dword that still holds the drop-target queue's
  place (DROPS_QUEUE_STEP), not from the hole. In a run nothing sounds
  (see "The hole's sound in a run"); a port that wants the original's
  sound has to do the same.
- Open: the ball's speed out of a hole; a hole without a picture.

### The display's animations

Read 2026-09-29 from the code (hints: ANIMS_STEP CODE:27A0E,
ANIM_FRAME CODE:27C2A, display opcodes 1 and 0Ch) and the four
modules' display streams (a throwaway script with tools/event_streams.py).
No run was made for it.

- Display opcode 1 plays an animation record once plus its fifth word
  more times (0 in most uses, up to 9); DISPLAY_RUN's opcode 7 waits
  for it. Opcode 0Ch sets a looping one that plays only while no
  opcode-1 animation and no display stream runs. ANIMS_STEP runs once a
  frame after DISPLAY_RUN and clears state+2A50h when the animation
  ends; DISPLAY_QUEUE clears it too when a stream of higher priority
  empties the queue.
- The frames come from `data\s00n\anims\allanims.mgl`: a dword count,
  a dword table, each frame three words (width, height, and in the
  first frame the animation's frame count) and run-length bytes (0..3
  the colours FCh..FFh, 4 colour 0, above 4 a skip). The buffer is 160
  bytes a line, 1400h bytes (160 x 32): the dot display, presumably.
  A step comes every +11h+1 frames (1..3 in the modules).
- Tables 1..4: opcode 1 names 37/45/63/44 records, opcode 0Ch 5/12/14/8;
  none has a next record at +0, so the code's side-by-side list is not
  used by the modules.
- Open: a run that shows the animation's frames on the screen (how the
  buffer reaches the screen is not followed); the tool could list the
  frames in allanims.mgl.

### The lamps, a drop-target bank and a hole in a run

Runs 2026-09-29, on macOS (the bundle's `ILLUSION.CFG` by `-put`, no
`-cue`; keys `106 space`, `112 enter`, `136 f1`, `137 enter`, both
Shift keys every 0.6 s from 138, Enter every 15 s from 152; a game of
three balls, over by about t=220; `build/drops/`). The run to t=330
took 47 s. [CODE:A11D] was 1D3170h, table 1's module at linear 2D40A0h
as in the Windows runs.

- The timed lamp (see "The lit records' timers and the lamps"): the
  skill shot's stream (header slot 27, `2 light_for @43D2 5`) lit
  record 43D2h at t=138.267 with +2Eh = 305 (5 x 61); it fell 7 each
  0.1 s; its lamp (light state 9958h) blinked (+1 FFh and 00h in turn,
  +3 counting 8..0 about one a frame); from +2Eh = 3Ch the lamp's bytes
  +0 and +2 were 0 (dark); CODE:2E952 unlit the record at t=142.614,
  4.35 s after. So the blinking and the dark last second are seen.
- The drop targets (see "The drop targets"): the ball hit target 5188h
  at t=205.128 and again 14 ms (one frame) later, target 5142h at
  209.887 and 209.901. The bank's stream (5128h +16h, stream 51CEh:
  light record 51E0h, block 5218h) was queued at both of the last two
  and 51E0h lit. So DROP_HIT runs twice for one hit, presumably while
  the ball's +6Ch stays; a port that pays a bank's points (table 1's
  are 0) or plays its sound should do the same. The bank has +4 bit 0:
  not raised, as read. Nothing of it looked at on the screen.
- A hole (see "The holes' eject"): count 4Ch twice (t=148.980,
  171.732), the ball out 0.223 and 0.227 s later: 16 frames. FFCEh not
  seen.
- tools/event_streams.py did not know the banks' streams: now they are
  roots too (a test with them); 80/90/68/84 event streams for tables
  1..4.
- Display opcode 1 came 20 times (the attract mode's and the game's),
  not looked at further.

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
(CODE:D890, D892, 9B8E, B922) are not looked at. The SVGA modes: 60 Hz,
FRAME_RATE 59 or 60 (see "The SVGA modes in a run").

### The player record in a run

Runs 2026-09-29, on macOS (as in "The lamps, a drop-target bank and a
hole in a run": the bundle's `ILLUSION.CFG` by `-put`, no `-cue`, the
same key file; `build/names/`; 31 to 45 s each). state+0D76h held D8B8h
(CODE:D8B8, linear 10E7E8h): player 1's record. `-dump 10E7E8 24
-dumpevery 0.25`, `-watch` on GAME_PHASE (linear 10DAFCh), `-log` on
TAKE_PAY, BONUS_ADD, BONUS_CLEAR, CODE:2BBC6 and the slot-15 handlers
1, 2, 5, 7, 8, 0Ah.

- Blind play, a game of three balls to t=217: 36 calls of TAKE_PAY, the score (the
  6 BCD bytes at +0, +1, +4..+7) rose to 3,165,000; the bonus (+8, +9,
  +0Ch..+0Fh) and bytes +10h..+14h stayed 0 all game, so no take of
  those paid a bonus (the screen at the lost balls not looked at). None
  of the handlers logged was reached.
- Poked at the first lost ball (CODE:2BBC6, t=155.54): bonus 12,345,
  word +12h 3, byte +10h 1. The screen showed "BONUS 12,345", then
  "EXTRA BALL"; the score went from 395,000 to 432,035 (3 x 12,345) by
  t=157.75; BONUS_CLEAR at 157.50 cleared the bonus and the word;
  byte +10h went to 0 and GAME_PHASE to 8 (by CODE:2BC0B), 4 again at
  the next Enter (t=168.14, CODE:2BBA8).
- The same with bytes +11h and +14h FFh and no extra ball: GAME_PHASE 6
  as usual; after BONUS_CLEAR the bonus 12,345 and the 3 stayed, both
  bytes were 0.

So score, bonus, multiplier, extra ball and the two "held" bytes are
checked as the hints name them; the "presumably" is gone from those
comments. Not checked: whether a multiplier shows on the display
(the screenshots at 155.4..162 have only the two texts), and the
handlers that set them (1, 2, 5, 8) in a run; no take in blind play
reaches them.

### The slot-16 counters' timers

Read 2026-09-29 from the code and table 1's streams, then two runs on
macOS (as in "The player record in a run"; `build/names/c1.txt`,
`c2.txt`). Hints: COUNTER_TIMERS (CODE:2CB69), handler 14h.

- The word +26h of a slot-16 counter, set by slot-15 handler 14h to
  seconds times FRAME_RATE, is read by COUNTER_TIMERS once a frame of
  play (the list at state+28E6h, from CODE:2B4B9): counted down, and at
  0 the counter's step (+30h/+34h) goes back to +28h/+2Ch and its value
  (+38h/+3Ch) to 0. The resets CODE:29FFA and 2A113 set +26h to 0 too.
  After a counter runs out the routine returns, so the counters after
  it miss that frame's count; no table has two timed counters, so it
  changes nothing as far as the data goes.
- Handler 14h is in three records: 579Ch (5 s) and 57D6h (10 s) of
  table 1 on counter 574Ah, 490Ch (12 s) of table 3 on 48BAh. Table 1's
  four zone streams 4A62h, 4AFCh, 4B8Ch, 4C20h each take one of records
  55FAh..5712h (handler 10h: the value up by the step, 1,000,000, and
  paid), then 579Ch and 57D6h, and light one of them for 5 or 10 s.
  So, as read: a chain of shots whose value grows by 1,000,000 a shot
  while each comes within 5 or 10 s of the last, and falls back to 0
  otherwise. Which zones those are on the table is not looked at.
- Blind play (a game of three balls to t=217): +26h stayed 0, the chain
  not reached.
- Forced (poked at the third TAKE_PAY, t=138.25: +26h 131h, step
  5,000,000, value 3,000,000): +26h fell one a frame, reached 0 at
  t=142.599 (4.35 s, 305 frames at 70 a second), and at that frame the
  step was 1,000,000 and the value 0 (CODE:2CBA6). COUNTER_TIMERS ran
  826 times up to t=150.

### Table 4's random awards

Read 2026-09-29 from the code and table 4's module, one run on macOS
for the counter (as in "The player record in a run";
`build/names/r1.txt`). Hints: CODE:2E51D, DRV_TICK, CODE:2F7DE.

- The number handler 1Ah picks by is the low byte of CODE:BAD4, which
  DRV_TICK counts once a frame: 838 writes by CODE:B9AA from t=130.11
  (the table's start) to 142, 70.5 a second. So the award depends on
  the frame of the take, not on a random-number generator.
- Table 4's record 5662h has 8 entries of 32 numbers each, all given
  once (flag bit 0): light 5728h (handler 1, an extra ball, and
  5,000,000 to the bonus when taken), take 4F3Ah (handler 6 on counter
  4F72h), take 5790h (10,000,000), 575Ch (1,000,000), 57C4h
  (25,000,000), and three streams with nothing but their end. A take
  pays only a record lit for the current player (event opcode 5), so
  whether the four takes pay depends on their state then; not looked
  at.
- Once all eight are given, the loop at CODE:2E56D (the number plus
  5Dh, coprime to 256) would never end. As read it is not reached:
  5662h is lit only by threshold 15 of counter 55F6h (handler 6 of
  record 541Eh counts it), which fires when the player's word +16h is
  15; that word is reset only at the game start (CODE:29FFA; the
  counter's flag bit 0 keeps it over balls at CODE:2A113) and the
  counter has no limit (word +4 0), so once per player and game, eight
  at most. The entries' bit 1 is cleared only at the game start
  (CODE:2A0C2) and is shared by the players. Not run: blind play does
  not reach threshold 15.

### The tilt in a run

Runs 2026-09-29, on macOS (as in "The player record in a run", but no
Shift keys: no flipper; `build/names/tilt*.txt`, `tl*.txt`). Hints:
TILT_STEP, GAME_PHASE, CODE:2B716.

- The nudge count state+2A78h falls 4 a frame, not 1: it falls 1 a
  call, and the call comes with CODE:144AD, which runs four times a
  frame (presumably; the path is not traced). A new press of Space
  adds TILT_STEP (100 with NORMAL), and at C8h or more state+2A75h is
  set. So NORMAL tilts at the third nudge within 25 frames of the first.
- Three Space presses 0.3 s apart (t=145.0): the count peaked at 77h,
  no tilt. 0.1 s apart: 5Fh after the first, A7h after the second, F3h at the third,
  state+2A75h FFh and GAME_PHASE 9 (CODE:2B918) at t=145.214; the
  display showed "TILT"; the ball was lost at 146.065 (no flipper was
  pressed in this run anyway), GAME_PHASE 5 and 6 in the same frame
  (1.96 and 2.64 s between them in the runs without a tilt): BONUS_ADD returns at once
  while tilted, no bonus. The next ball was served as usual.
- Not seen: whether the flippers work in phase 9 (the routine reads no
  flipper keys, as read); the score's fate, as it was 0 here.
- Phase 0: see "Game phase 0".

### Game phase 0

Read 2026-09-29 from the code, one run on macOS (as in "The tilt in a
run": keys `106 space`, `112 enter`, `136 f1`, `137 enter`, `146 esc`,
`150 esc`; `-poke 12C69E#300 10DAFC "00 00"`, the 300th pass of play's
routine; `-watch 10DAFC`, `-log 10CA2E`; `build/phase0/`, 20 s). Hints:
GAME_PHASE, CODE:BAFE, CODE:BB0A.

- No instruction writes 0 to GAME_PHASE: its 18 writes are the
  constants 1..9, and the image's 0 is replaced by 1 at CODE:B928
  before the round loop. The run's `-watch` saw 0 only at the image's
  loading (t=2.28) and at the poke.
- The routine (CODE:BAFE) waits for the retrace until port 60h reads 1
  (Esc down) and returns; the phase stays 0, so it is called again. In
  the run: 70 passes a second from the poke (t=145.156) to the end
  (t=156), AL 1 at both Esc presses and the loop went on; the
  screenshots at 149 and 155 are identical. So phase 0 hangs the game
  (interrupts go on; the music not listened to).
- After it, behind the text "9090", code with no reference (CODE:BB0E):
  keys 1..4 move two words of the state, read nowhere else, the screen
  scrolls to the second, and a ball-like position (y x 150h + x, as
  CODE:269D7 computes a ball's +72h) goes to DS:72h. A debug tool,
  presumably, and phase 0 a debug pause; both dead in this release.

### The SVGA modes in a run

Read 2026-09-29 from the code, then runs on macOS with doskit's new
VESA (doskit: `4F00h`..`4F03h`, modes 100h, 101h, 103h with an SVGA
card's registers; its selftest checks them in VGAMODE.EXE). Hints:
OPT_SVGA_MODE, SVGA_CHECK, SVGA_FIND, the comment on MODE_SVGA640.

- At the start SVGA_CHECK (CODE:0753) looks for a mode when the
  resolution is SVGA: an S3 BIOS, else VESA (4F00h and the block's
  "VESA": modes 101h / 103h), else the BIOS modes 14h..7Fh one by one
  with a check of the registers. Nothing found: back to VGA 360x350 and
  the configuration file written so. The runner had no VESA, so SVGA
  could not be chosen in it before.
- The SVGA mode routines set that mode and then program the card as VGA
  360 does: planar, 84 bytes (336 pixels) a line, the split for the
  32-line display. So SVGA shows more of the table's height (448 or 568
  lines instead of 318), not more width; on a real monitor the 336
  pixels presumably sit in the middle of the 640 or 800 (the retrace is
  moved; not seen, the runner shows only the pixels displayed).
- Runs (keys `106 space`, `112 enter`, `136 f1`, `137 enter`; `-poke
  101683 100FD2 01` or `02`: OPT_RESOLUTION at SVGA_CHECK's first
  instruction; `build/svga/`, 20 s each): INT 10h AX=4F00h at t=2.4,
  OPT_SVGA_MODE 1; at the table's start (t=124.04) the mode routine,
  FRAME_RATE 3Ch, then MEASURE_RATE 3Bh (640x480, 59.94 Hz) or 3Ch
  (800x600, 60.3 Hz); the table in 336x480 and 336x600 pictures (the
  ball, the display's "DON'T MOVE"); the runner's `-vgastate` at the
  end of the 800x600 run: mode 103h, CR 13h 2Ah, line compare 567.
- Not done: a game played to its end in SVGA; the VGA 320x240 mode (its
  routine sets FRAME_RATE 3Ch too; no VESA needed, not run); S3's and
  other cards' own modes (the runner has only VESA).

### The table angle and the ball save in a run

Runs 2026-09-29, on macOS (as in "The player record in a run": the
bundle's `ILLUSION.CFG` by `-put`, no `-cue`; keys `106 space`, `112
enter`, `136 f1`, `137 enter`, no flipper; `build/angle/`, 25 to 40 s
each). Linear addresses (the loader's hints do not name the game's):
CODE:x is 100F30h + x. Hints: SLOPE_Y, SERVE_SECONDS, BALL_SAVE, the
comment on event opcode 0Bh (CODE:2D210).

- The table angle: `OPT_ANGLE` poked to 0..4 at `OPTIONS_APPLY` (`-poke
  10C210 100FCE 0n`), the first ball's record (state+10AEh, linear
  10EB1Ch) dumped every 7 ms. `SLOPE_Y` was 3, 4, 5, 1, 2 and so was the
  record's +3Eh. In the ball's free fall into the launch lane at the
  serve (t=136.01 to 136.14) its word +10h grew by 24, 32, 40, 8, 16 a
  frame: 8 x `SLOPE_Y`. So the option is the pull down the table, as
  read; the 8 is presumably CODE:1352D's add running 8 times a frame
  (not counted). How the ball's path changes was not compared (the
  first lost ball came at 155.95, 159.18 and 148.03 with NORMAL, VERY
  HIGH and VERY LOW: blind play, so says nothing).
- `SERVE_SECONDS` is the length of a ball save: the serve (header slot
  34's record, CODE:2B1DC) writes 10 x 61 = 610 frames to state+0D3Eh
  (now `BALL_SAVE`), and play counts it down one a frame from the
  launch (t=138.269). A ball lost while it is not 0 is served again:
  GAME_PHASE 7 ("DON'T MOVE", already known), 1.3 s, then the ball is
  launched with no key and play goes on with the count where it was.
  Blind play on table 1 lost the ball at 140.23, 143.05 and 146.05,
  each saved; the save ran out about 147, and the loss at 155.95 was the
  real one. With `SERVE_SECONDS` poked to 30 (at the serve, `-poke
  12C10C 10E8B0 "1E 00"`): six saves up to t=162, the 1830 frames ran
  out at 176.27, 38.0 s after the launch as phase 7 does not count,
  the ball lost for good at 177.46.
- Read, not looked at: the save's lamp (header slot 25's second
  pointer) blinks every 4 frames while more than 100 frames are left,
  every frame down to 51, and is dark for the last 50. Event opcode 0Bh
  sets the same count (a save of its word's seconds); not run.

### The multiball in a run

Found 2026-09-29, on Linux, from the modules (tools/event_streams.py)
and runs (keys `106 space`, `112 enter`, `136 f1`, `137 enter`, no
flipper; the GOG `ILLUSION.CFG` by `-put`, no `-cue`; `build/multiball/`,
30 to 45 s each). Hints: MULTIBALL_CAP, OPT_MULTIBALL, MULTIBALL_ON,
BALLS_ON_TABLE, BALLS_TO_SERVE.

- Header slots 43 and 44 (+0ACh, +0B0h) point to event opcode 1Bh
  (`multiball`) commands in mode streams, and word +2 of the record is
  the command's ball count (its word operand): table 1 8FC8h (stream
  8F62h, from event 4356h, slot-15 record 439Ah's handler 11h; 3 in
  the file) and 8598h (stream 8530h, threshold 8 of counter 421Eh; 6);
  table 3 6EA2h (stream 6E7Eh; 3); table 4 809Ah (803Ah; 3) and 8786h
  (8734h; 3). Table 2's two slots and table 3's slot 44 point to zeros,
  which MULTIBALL_CAP leaves alone (its numbers there are 0). The other
  opcode 1Bh commands (2 or 3 balls, 13 of them over the four tables)
  are not touched.
- So MULTIBALL_CAP sets those counts to min(table's number, option),
  raising a count as well as lowering it: with SIX (the GOG
  configuration) table 1's 3 and 6 become 6 and 4. Seen: `-log` at
  MULTIBALL_CAP (t=129.597) and `-dump` of the two words (module base
  linear 2D40A0h, as in the Windows runs): 6 and 4 after it; with
  OPT_MULTIBALL poked to 1 (THREE) at its first instruction, 3 and 3.
- Stream 8F62h forced: the earlier forcing of threshold 1 (see "A
  mode's track in a run": record 4362h lit, zone 3447h over the table,
  at CODE:2E0F6's first pass) and event 42DEh's pointer (linear 2D8384h:
  loaded as 1D3170h + the module offset) poked to 8F62h. The zone is
  put back at event opcode 1Bh's first call (CODE:2D70E, linear
  12E63Eh); left over the table (a first run), the multiball stalled
  from t=151.5 with one ball and five to serve (every ball caught by the
  zone's object, presumably; not looked at). State+0D2Eh
  to 0D3Fh dumped every 0.25 s:
  - opcode 1Bh at t=145.17, after the stream's 7 s wait; ball_save 1Eh
    before it: BALL_SAVE 1823 at 145.25;
  - SIX: MULTIBALL_ON FFh, BALLS_ON_TABLE + BALLS_TO_SERVE 6 while
    the save ran (lost balls go back to be served), 6 on the table at
    163.0; the save out at about 171.3 (1830 frames at 70 a second,
    26.1 s: the game-second of "The frame rate"); the six drained with
    no flipper by 175.25, MULTIBALL_ON 0 at one ball (174.75); ball 2
    at 178.5;
  - THREE: the sum 3, at most 3 on the table, MULTIBALL_ON 0 at one
    ball (172.25).
- Not done: tables 3 and 4 and table 1's stream 8530h in a run (read
  only); who clears MULTIBALL_ON (seen cleared at one ball; the writer
  not looked at); the other bytes of the dump (state+0D2Fh, 0D30h,
  0D34h, 0D3Ch, 0D3Dh) are left unnamed; the screenshots
  (`build/multiball/g_*.png`) show one ball at a time as the view
  follows a ball, so the counts come from the dump, not the picture.

### The Info page and the greetings page

Found 2026-09-29, on Linux, from the code and runs (the GOG
`ILLUSION.CFG` by `-put`; `build/greet/`). Hints: INFODATA_SEL,
TINYFONT_SEL, TEXT_POS, KEY_HISTORY, INFO_PAGE, GREETINGS_PAGE,
GREETINGS_TEXT, the comments at CODE:4532 and 505C.

- The selector in CODE:3B4B is written by `POP WORD PTR [3B4B]` (CODE:4FD6,
  76EE) after INT 94h AH=1 loads `chooser\infodata.mgl`; CODE:3B4D the
  same for `tinyfont.fnt`. The earlier search looked for MOVs only.
- TEXT_POS (CODE:3B58) is written by two routines that differ in DS.
  INFO_PAGE (CODE:4504) stores 430Ah + a dword from INFODATA.MGL (+4 +
  4 x the page, the file's first dword the count 4), read with DS =
  INFODATA_SEL (CODE:41A8, 3FF3): an offset in that file, left a
  number. GREETINGS_PAGE (CODE:4871) stores 46AFh, read with DS
  unchanged (CODE:4197, 405E): the text in CODE before it, now
  GREETINGS_TEXT (`dptr` at CODE:4886; build.py IDENTICAL).
- GREETINGS_PAGE is reached only when the dword KEY_HISTORY
  (CODE:1538), the last four scan codes of the chooser's keyboard
  handler CODE:340A, is 0F3A2A1Dh: Tab, Caps Lock, Left Shift, Left
  Ctrl, in that order. The handler is installed (CODE:33B8) as Space
  brings up the table menu (t=100.56); CODE:505C, the menu's start, is
  entered at the next key and checks it (CODE:506C), and the menu loop
  checks it too (CODE:3943, sets CODE:1904; not followed).
- Runs: `-key 100 space`, then `106 tab`, `106.5 3a`, `107 lshift`,
  `107.5 lctrl`: KEY_HISTORY 0F3A2A1Dh at 108.0, CODE:505C and
  GREETINGS_PAGE at 108.53, the page on the screen from about 109 (the
  greetings of FrontLine Design, in the small font). `100 space`, `106
  enter`, `110 right`, `111 enter`: the menu loop at 106.76, INFO_PAGE
  at 111.56, the Law 'N Justice page on screen with its picture and
  high scores. A key before the menu loop starts is taken for the
  menu's start (Info is right of the list, not below it).
- The menu's keys (MENU_KEYS, CODE:3492, one a frame from the ring at
  CODE:1906): Down/Up move MENU_ROW (CODE:18F9, the table, 0..3),
  Right/Left MENU_INFO (CODE:18FB); Enter or Space with MENU_ON starts
  the fade out (MENU_FADE, CODE:18FE), whose end sets MENU_DONE
  (CODE:1916) and ends MENU_LOOP (CODE:3969); F1..F4 choose table 1..4
  at once; Esc sets ESC_KEY (CODE:18FF). After MENU_LOOP, CODE:505C calls
  INFO_PAGE for MENU_ROW while MENU_INFO is set, else goes on with
  MENU_ROW in AL at CODE:526D (FFh on Esc, the way out to DOS).
- The menu has a stage before it (CHOOSER_WAIT, CODE:38C6): the backdrop
  with changing captions and no list, from t=104.21 after Space at 100
  (which only ends the credits). Any of Esc, Enter, Space, or
  KEY_HISTORY's four keys, sets WAIT_END (not F1..F4: they are dropped
  while MENU_ON is 0, seen in a run with F1 at 130 and 132; corrected
  after the first commit said otherwise) (CODE:1904, what the earlier
  note called "the menu loop" there); a few frames later the list comes
  up (CODE:505C). That is why a key before the list is not taken as a
  choice.
- The Info and greetings pages read keys through PAGE_KEYS (CODE:3465),
  which heeds only Esc: Esc fades them out (33 frames) and returns to
  MENU_LOOP with the cursor on the same row.
- Runs (`build/menu/`, 2026-09-29, the GOG `ILLUSION.CFG` by `-put`):
  `100 space, 106 enter, 108 down, 109 down, 110 right, 111 enter, 118
  esc`: MENU_ROW 1, 2 at 108.00, 109.01; INFO_PAGE at 111.56, the
  Extreme Sports page ("Press ESC to exit."); Esc at 118.02, MENU_LOOP
  at 118.73, Extreme Sports still marked. The greetings keys as above
  and `112 esc`: CODE:505C and GREETINGS_PAGE at 108.19 (the earlier
  run's 108.53 not accounted for), MENU_LOOP again at 112.74. `100
  space, 106 enter, 108 f3`: CODE:35F8 at 108.00, AL=2 at CODE:526D at
  108.57, Extreme Sports on screen at 139.
- The captions: all text on the chooser's backdrop (the credits in
  CHOOSER_WAIT, the table list in MENU_LOOP) is a caption record of 24h
  bytes, six far pointers to routines CAPTION_BUILD (CODE:302E) makes
  from a layout in CODE (CODE:1AE6..226E; the command format in the
  hints there). CAPTION (CODE:1890) is the one drawn. CHOOSER_WAIT steps
  through CAPTIONS (CODE:1898, 11 records); MENU_LOOP takes the list
  picture of MENU_ROW from one of three tables: the table boxed, Info
  boxed, or no box while MENU_BLINK (CODE:18FA) is 1, so the box blinks.
  The list scrolls so that the marked row is always at the box's y.
- The backdrop (ATTRACT_STEP, CODE:36F4): a new palette every 300
  frames, and after nine a new backdrop, one of three in turn.
- Run with no keys (`build/menu/e.txt`, screenshots every 3 s from 90):
  the chooser (CODE:4FF9) at 107.70, CHOOSER_WAIT at 107.87; a caption
  every 5.05 s from 109.63 (Pinball Illusions at 111, Production at 147,
  Press Space Bar at 159, round again at 165.86); palette stages 5.03 s
  apart from 112.90; the cubes gave way to crosses at 148.42. Without
  Space at 100 the credits run 3.7 s longer.
- The pointer tables (CAPTIONS, MENU_PICS...) are left numbers: the
  kit's `words` hint with CODE as the target also seeds code there, and
  the records they point at are zeros in the image. A `words` form
  without code seeding would be a kit change (AGENTS.md rule 7).
- Not done: the routines CODE:2E35 makes (presumably compiled drawing
  code, not read) and why there are six of them per caption; why
  ATTRACT_START is reached at 104.21, before CODE:4FF9 (not looked at).
- A note for byte searches: CODE:x is file offset x + 24h in
  ILLUSION.386 (checked on CODE:4532, 4886), not + 28h.

### Table 1's shooting game in a run

Found 2026-09-29, on Linux, from table 1's module and runs (`build/vm/`;
the details in docs/bpc-module.md, "Table 1's shooting game"). Table
1's opcode-14h object (module 9A46h) is a game on the display: bad
guys in four windows, a crosshair moved by the flippers, a bad guy in
the crosshair's window shot when it aims; four lives; 30 hits end the
game with 50,000,000 more, the game's score paid to the player and the
extra ball lit (the 25-hit extra ball of the code is never given, see
"Table 1's shooting game when the lives run out").

- Runs as in "The table angle and the ball save in a run" (`-put` of
  the GOG `ILLUSION.CFG`, keys `106 space`, `112 enter`, `136 f1`, `137
  enter`), with the two pokes of "A mode's track in a run" and the
  counter's word +16h poked to 4 at the same moment (`-poke 12F026#1
  2D82D4 "04 00"`; the count-up there makes it 5, threshold 5's mode;
  5 gave 6). Module base 2D40A0h as before; the state record 0A19Fh is
  linear 2DE23Fh.
- No flipper in the game: the start at t=145.04, lives 4, 3, 2, 1 at
  146.2..146.6, one hit at 147.8 (the crosshair stays on window 1), lives
  0 at 148.0, 206 updates in 2.93 s (70 a second, one a frame). The
  display: "GIMME YOUR BEST SHOT / TO CLEAR THE STREET" at 141, the
  street at 146.
- Lives poked to 99 at the first update (`-poke 2DDBC5#1 2DE241 "63
  00"`) and the flippers pressed every 1.1 s (left) and 1.7 s (right)
  from 145.2: 30 hits by 177.5, 40 lives lost; `+8` FFh at the 25th hit
  (173.45); the game's score (BCD, `+11h`) counted 1 a hit (21 at 21
  hits) and was 80 after the 30th: 80,000,000. The player's score
  (linear 10E7E8h, byte +7) went from 0 to 80h at 178.0 (and +6 from 10h
  to 20h at 179.5, presumably a take; not looked at). The display: "EXCELLENT" at 178, a number
  at 180 (not legible at the shot's size), "EXTRA BALL IS LIT" at 184.
- Not run: which `vm_*` picture is which, the extra ball collected
  afterwards. (The lives-out paths: next section.)
- Found on the way: the key times of the earlier Windows runs (`130
  f1`, `131 enter`) do not reach a table with `-put` of the GOG
  configuration (F1 in CHOOSER_WAIT is dropped, see "The Info page and
  the greetings page"); `seq` writes decimal commas under a German
  locale, which the runner's `-keys` does not read (`LC_ALL=C`).

### Table 1's shooting game when the lives run out

Found 2026-09-29, on Linux, from table 1's module, the main program and
runs (`build/vmend/`; keys and the three pokes of "Table 1's shooting
game in a run", the state poked at the first update, `-poke 2DDBC5#1
2DE23F ...`). Hint: the comment at CODE:30368.

- The code (module 9D3Eh..9DD2h): the hits go through host vector +18h
  (CODE:30368, a word to decimal text) into "YOU SHOT 00 BAD GUYS";
  then the module reloads its state pointer from [CODE:0004] and tests
  byte +8 (FFh from the 25th hit, set at module 9EBAh, read nowhere
  else): FFh would queue event stream 5F96h (`light @5FC6`, the extra
  ball), otherwise 5FA2h (`display @6030`). CODE:30368 uses CODE:0004
  as a work cell and leaves it pointing into its digit tables
  (CODE:30585 + 2n - 2 for 1..15 hits, CODE:305A5 for 16..29), where
  the byte at +8 is not FFh for any count 1..29 (read from the image;
  the tables are only read). So in the original the extra ball is lit
  only by 30 hits; 25..29 hits end as fewer do.
- Runs: hits 7, lives 1: lives 0 between 146.0 and 146.5, "YOU SHOT 07
  BAD GUYS" and "0" on the display at 147. Hits 26, `+8` FFh, lives 1:
  the same with "26"; the shots of the two runs differ only in the
  number, from 148 not at all; the event queue (CODE:2FE8E) and the
  display queue (CODE:2FEDB) called at the same moments (146.155,
  146.165); module 9DAFh (the 5FA2h branch) reached, 9D8Bh not, SI at
  the test 05A5h (the runner prints 16 bits: CODE:305A5, presumably);
  record 5FC6h's byte +1 not written up to t=149.
- Hits 7, lives 1 and the game's score's bytes +0Eh..+11h poked to
  `00 07 00 00`: "YOU SHOT 07 BAD GUYS" and "700" on the display at
  147; the player's score (linear 10E7E8h) the same as in the run with
  score 0 at every half second up to 149.5 (it counts up from 147.5 in
  both, something else of the mode's end; not looked at). So the game's
  score is shown, not paid. How the score's bytes map to the digits
  shown is not worked out.

### Table 4's sea game in a run

Found 2026-09-29, on Linux, from table 4's module (read in part) and runs
(`build/vm4/`; the details in docs/bpc-module.md, "Table 4's sea game").
Table 4's opcode-14h object (module 97E5h) is a boat on the display,
steered with the flippers past rocks; bonuses pay 5,000,000 or
10,000,000, the sixteenth is the extra ball and ends the game; Enter,
once a game, clears the rocks and for 150 frames no new ones come; a
crash ends the game with "ITEM COLLECTED / FISH".

- Keys to table 4 with `-put` of the GOG configuration: `106 space`,
  Down at 110, 110.5, 111, `112 enter` (the list is up already after
  Space here), `136 f1`, `137 enter`. Module base: [CODE:A11D] =
  19C490h, linear 29D3C0h. Forced as table 1's: at the first count-up
  (linear 12F026h, t=137.32) record 5EA2h lit for player 0 (`2A3263
  01`), zone 33FFh over the table (`2A07BF "00 00 00 00 50 01 3C 02"`),
  counter 5F60h's word +16h 1 (`2A3336 "01 00"`, counted to 2); state+F1
  cleared at the object's start (`-poke 2A6BAD#1 10DB5F 00`).
- The game started at t=141.82 in every run. Without the clearing
  poke the serve's Enter ran Enter's script at once and the game ended
  after 322 frames. No keys: the crash (9EC8h) at 143.14, the end
  (9C44h) at 143.92. Flippers every 0.9 s (left) and 1.3 s (right): the
  crash at 150.92. The same and Enter at 145, 150, 155, 160: Enter's
  script at 145.01, its end (9BC8h) at 145.37, the display empty but
  for the boat at 146; a bonus (9F68h) at 148.51, the player's score
  byte +7 from 0 to 5 (5,000,000); the later Enters did nothing; the
  crash at 159.47, "ITEM COLLECTED" and "FISH" with a picture at 161.
- Read later the same day (docs/bpc-module.md, "Table 4's sea game",
  rewritten): how rows are made (eight columns, a kind and a row each),
  the course (a step on only per bonus: 8 x 5,000,000, 7 x 10,000,000,
  then the extra ball), the random number (the PIT's counter 0 in it),
  the steering, the arrow to a bonus. Corrected: immunity stops new
  rocks, it does not stop a crash.
- Runs (`build/vm4b/`, the same keys and pokes; module x is linear
  29D3C0h + x): a 10,000,000 bonus put in the boat's column at the
  first update (`-poke 2A6C38#1 2A807B 4E`: column 1, row 14): A053h
  at 141.89 (the fifth frame), the player's score byte +7 from 00 to
  10 in the same frame; a random 5,000,000 bonus (9F68h) at 144.62
  with no key pressed; the course index 8 at the end. The extra ball
  (`... 2A807B 6E`): 9F1Dh at 141.89, the update not called again,
  "EXTRA BALL" on the display at 142.5 and 144, no crash, 9C44h not
  reached. Steering (ACCCh..ACD3h dumped every 0.05 s, left flipper at
  142.3, right at 143.0 and 143.6): lane 0 to 7 at once at 142.35, the
  position 70h down by 4 a frame to 0 by 142.75; the right: the
  position up from 0Ch to 7Ch and the lane 7 to 0 at 143.50, again 0
  to 1 at 144.05; the scroll +2 a frame, a row every 4 frames; the
  crash (9EC8h) at 145.07.
- The drawing read the same day (docs/bpc-module.md, "Table 4's sea
  game": five of the eight columns around the boat in perspective, a
  swell table, the arrow). Runs (`build/vm4b/`): a 5,000,000 bonus put
  three columns right of the boat (`-poke 2A6C38#1 2A807E 21`): ACB6h
  0Ch from 142.0, a right arrow at the display's right end, the boat in
  the middle, a band at the bottom at 141.9 and 142.5 but not 142.1
  (the swell). The course poked to step 8 (`... 2A8094 20`): speed 8, a
  row every 2 frames (0.028 s), the scroll +4 a frame. A rock at row 14
  in the boat's column with immunity poked to 150 (`... 2A807B 0E`,
  `... 2A808B 96`): the crash at 141.89 all the same.
- Not checked: which file the pictures come from, what each picture
  looks like beyond the boat, the arrow and the band.

### Table 3's opcode-14h record

Found 2026-09-29, on Linux, from table 3's module data (no run; the
details in docs/bpc-module.md, "Deferred event dispatch"). The earlier
analysis listed a fourth opcode-14h command in table 3 (8E94h, object
8EA0h with null methods, "a shared placeholder"). It is not a command:
8E94h is the light ID (14h, word +1Ch) of the light state 8E78h of
slot-14 group descriptor 8E6Eh, and 8E96h is the next descriptor, whose
first light state 8EA0h (light 15h) eight slot-15 records point at by
+4. The earlier scanner's rule (a word 14h before a relocated dword)
gives just this one hit in table 3 and the three real commands in
tables 1, 2 and 4, which tools/event_streams.py reaches as
`module_call`; its 68 event streams of table 3 have none. So table 3
has no game on the display of this kind. Not seen in a run: that table
3 never enters the opcode-14h handler (CODE:2DBAD).

### The hole's sound in a run

Found 2026-09-29, on Linux, from the code and runs (`build/hole/`; keys
of "The lamps, a drop-target bank and a hole in a run": `106 space`,
`112 enter`, `136 f1`, `137 enter`, both Shift keys every 0.6 s from
138, Enter every 15 s from 152; `-put` of the GOG `ILLUSION.CFG`, no
`-cue`; 51 s to t=230). Hint: the comment at CODE:30996 (see "The
holes' eject" for the slip).

- Table 1's hole at module 4F18h (bit 1 clear, a picture at +30h)
  ejected twice; the last-50-frames read of its sound record (`-log` on
  CODE:30B87 and CODE:30B8A, linear 131AB7h and 131ABAh) came 7 times
  each, at counts 30h, 28h, .., 0 (t=149.37..150.06 and
  172.13..172.81). At a `-break` there [CODE:0004] was F920h, the
  drop-target queue's empty end, and CODE:F930 held 0.
- So in these ejects nothing sounds, and the JE to the RET on a 0
  record also skips that frame's draw of the flickering picture (what
  that looks like on the screen not checked). The hole's own +10h is
  module 11BBFh, a type-2 record (read from T001.BPC and its relocations;
  22 relocated pointers of the module name it); a fix would play it 7
  times an eject.
- The queue (state+2A60h) grows down from F920h (CODE:2CA4D, CODE:30800
  write below it, DROPS_QUEUE_STEP reads up to it); nothing writes at
  or above F920h. With one or two entries left the read still gives 0;
  with three or more it lands in the queue's own entries and hands
  CODE:9F2C a pointer made of their bytes (read, not run: a bank raised
  while a hole's picture flickers). For the port: by default no sound
  and no draw on those frames.

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

### The image in memory at its entry

For the port's memory (doskit's `pmem.h`, 2026-09-29): a run (Linux,
the GOG `ILLUSION.CFG` by `-put`, arguments `ILLUSION.EXE
'C:\ILLUSION.CFG' /`) stopped at ENTRY, linear 1011D3h (reached at
t=2.331472 as 0014:02A3, DS 18h, SS 58h), with doskit's new `-mem`
(`build/pm/entry.mem`):

- the image is at linear 100F30h, its 46480h bytes equal to the file's
  with all 24 selector relocations set to 1Ch (checked byte for byte);
  so descriptor 0's selector is 1Ch, not the CS 14h the code runs with
  (both have base 100F30h, presumably; the descriptor tables not read);
- the 8 bytes after the image are not all 0: a dword 1 and a dword
  EA8C30h at 1473B0h, presumably pMAX's heap header (1473B0h + EA8C30h
  is FEFFE0h, near the 16 MB end; not followed); 100000h..100F2Fh holds
  pMAX's own bytes (3219 not 0). Neither is the program's; the port does
  not model them;
- `memcmp.py src/ILLUSION.hints A B --base 100F30` compares CODE (and
  the empty TAIL) of two such dumps.

Not found yet: where INT 92h's memory (the 2F0800h bytes ENTRY asks
for) lies, which the port needs where the game keeps data outside the
image.

### The configuration file's options

What ENTRY changes before SETUP_ARGS (CODE:32F39, linear 133E69h; runs
2026-09-29 as above, `-mem` at both, memcmp.py): only CODE_SEL (1Ch)
and CFG_NAME (`C:\ILLUSION.CFG` and its NUL), and OPTIONS by INT 94h
AH=7, all 0 with the GOG file.

The file's 200h bytes at +20h (all `SN95` in the GOG file,
docs/configuration.md) are the options stored chained: stored byte i =
option byte i XOR stored byte i-4, the first four XOR `S`, `N`, `9`,
`5`; so options all 0 give `SN95` throughout. Found with crafted files
put in place of the GOG one (`-put`): options 01 02 03 01 00 01 00 55
XORed with `SN95` alone read back as 01 02 03 01 01 03 03 54 (each
byte XOR the one four before); then 200h random bytes stored by the
chained rule read back exactly (OPTIONS at SETUP_ARGS). Whether INT 94h
AH=6 writes them the same way is presumed, not checked; the header's
first 20h bytes (the driver's name, then bytes not read) are not
looked at here.

### SETUP_ARGS in a run

The same run stopped at SVGA_CHECK (linear 101683h, t=2.425941; `-mem`,
memcmp.py against the dump at SETUP_ARGS): SETUP_ARGS leaves 28 bytes
changed in CODE, all accounted for by the new names:

- CFG_HEADER (CODE:30DB2): the GOG file's first 20h bytes, by INT 94h
  AH=5;
- SETSOUND_TEXTS (CODE:331C0): offset 0Ch (the English table, LANGUAGE
  0), selector 4; YES_KEYS (CODE:331BC) 7959h, "Yy";
- SETSOUND.DAT itself lies at linear 1473C0h, all 12,823 bytes, freed
  but not cleared: 10h after the image's end, behind a 10h-byte header
  at 1473B0h (01 00 00 00, 30 8C EA 00, then 3290Fh and 1Ch: the
  file-name argument's offset and selector, presumably; the second
  dword was there before the load too). pMAX's heap, not the program's;
  the port puts its blocks the same way (port/src/pmax.c), only this
  first one seen.

`/?` in the original (`ILLUSION.EXE C:\ILLUSION.CFG /?`) and in the port
(`-opt ?`): the same help text ("There are several ways to run the
game" ...), compared line by line without the blank lines, which the
runner's `con:` lines leave out. The original's exit code 32 (see
"The loader in the runner") is not the port's (0), not looked into.

### The CD check and the driver's start in a run

CODE:757D first calls CODE:7082 through the checksummed jump CODE:4CF2
(the dword at CODE:9073 less the byte sum at CODE:6A20, see the hints).
CODE:7082: MSCDEX (CODE:35B52, CODE:35B7D, CODE:35C58), the
configuration header again to CODE:6360 (INT 94h AH=5), the driver it
names loaded by INT 94h AH=1 and made callable (INT 93h AH=8, DX
409Ah), the host callback table at CODE:618D given CS (CODE:61D9), the
driver's command 0 (FS:EDI the table, ES:EBX CODE:6360) and command 4
(slot 0, `intro\MOD.INT`, CODE:80BC); CODE:1550 = CODE:4CF2 at the end.

Runs 2026-09-29 (GOG `ILLUSION.CFG`, `-cue`; stopped after the call,
CODE:75B6 = linear 1084E6h, t=6.655899; `-mem`, memcmp.py against the
dump at CODE:757D): 257 bytes in 29 runs change in CODE, video memory
not at all:

- CODE:1550 = 4CF2h; the eleven selector words of the table at
  CODE:618D 1Ch -> 14h; CODE:61D3 dword 47F88h and CODE:61D7 word 44h
  (the host callback 6's cursor and the loaded file's selector: MOD.INT
  is 47F88h bytes);
- CODE:6354 48h (INT 93h AH=5's selector, the video memory's,
  presumably), CODE:6358 4 (the driver's block), CODE:635E 1Ch (DS),
  CODE:6360.. the header, CODE:8134/8138 the driver's entry 0:0Ch (the
  code selector INT 93h AH=8 made);
- the CD's state: CODE:35EDE..35F0A, the track table CODE:35F1F..35FEA,
  CODE:3631B 3 (drive D:).

The same run with `NOSOUND.SDR` in the header (a copy of the GOG file,
the name changed): the image at the same point differs from SB16's only
in the name (CFG_HEADER and CODE:6360), CODE:61D3/61D7 included. So what
the start leaves in the image does not depend on the driver: the
module is loaded through the host's callbacks (6 and 7), which are the
game's code.

pMAX's heap, from the same runs (`-intwatch 92`, the memory dumps): the
driver's block at linear 1473C0h again (selector 4), MOD.INT at
FA8060h, near the top of memory, behind a header `01 FF 00 00`, the
size 47F90h, then 629Ch and 1Ch 0002h (the name's offset and selector
and, presumably, the allocation policy 2 of host callback 1: INT 92h
AH=8 BL=2 before the allocation). So pMAX allocates from the bottom
(policy 0) and from the top (policy 2); the selectors seen are 4, 0Ch
(the alias), 44h (MOD.INT) and 48h (video). How pMAX numbers its
selectors is not worked out; the port's heap (port/src/pmax.c) knows
only the bottom-up case, which matched for SETSOUND.DAT and
LOADING_PIC's block.

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
     it (the frame rate done 2026-09-29, see "The frame rate"; the
     slot-15 update CODE:2E8CD done 2026-09-29, see "The lit records'
     timers and the lamps"; opcode 4's objects done 2026-09-29, see "The
     drop targets"; the hole eject done 2026-09-29, see "The holes'
     eject"; the animations of display opcodes 1 and 0Ch done 2026-09-29,
     see "The display's animations"). These were what a port of the two
     interpreters needed.
   - done 2026-09-29: the slot-15 handlers (see "The slot-15
     handlers"). Done 2026-09-29: a run that checks the names
     (score, bonus, multiplier, extra ball; see "The player record in a
     run"). Done 2026-09-29: who reads a slot-16 counter's word +26h (see "The
     slot-16 counters' timers"). Done 2026-09-29: table 4's
     random awards and how often CODE:BAD4 is counted (see "Table 4's
     random awards").
   - done 2026-09-29: table 2's music (see "The audio records and
     table 2's music chooser"); its run done 2026-09-29 (see "Table 2's
     chooser in a run"). Open from it: table 1's and 4's opcode-14h
     objects (table 1's done 2026-09-29, see "Table 1's shooting game
     in a run"; table 4's, see "Table 4's sea game in a run"; table 3
     has none, see "Table 3's opcode-14h record"). Done 2026-09-29: a jingle (driver command 0Ah) requested
     in a run (see "A jingle in a run"). Done 2026-09-29: why the
     runner's Sound Blaster played nothing (see "The Sound Blaster in
     the runner"). Done 2026-09-29: the jingle run again with the fix
     (see "The jingle in the WAV"; the runner's breakpoints fixed on
     the way). Open from it: the WAV listened to; what the number in a
     record's positive word +2 means. Done 2026-09-29: the jingle alone
     (see "The jingle alone": it takes the module music's place for
     1.78 s, the module goes on after it). Done 2026-09-29: the CD's
     audio in a WAV (see "The CD in a WAV"). Done 2026-09-29: the CD's
     volume around a jingle and how the driver ends a jingle (see "The
     CD's volume around a jingle"; a runner bug fixed on the way). Open
     from it: a track's time running out during a jingle. Done
     2026-09-29: bit 1 of a record's byte +0Ah (see "Bit 1 of a record's
     byte +0Ah": read by no one).
   - done 2026-09-29: Stage 1 (item 2), the offsets among the 32-bit
     immediates (see "Offsets among the immediates"). Open from it: the
     selector in CODE:3B4B (done 2026-09-29, see "The Info page and the
     greetings page"); names for the routines found in these
     sessions (METHOD.md), keeping build.py IDENTICAL (22 given
     2026-09-29, see "The gaps", the end). Done 2026-09-29:
     the self-patched call at CODE:298C5 (see "The self-patched call in
     a run": not patched in runs). Done 2026-09-29: the options screen's
     argument (see "The self-patched call in a run", the end); what Esc
     does (see "Esc"). Done 2026-09-29: the other option bytes (see "The
options"; open from it: the multiball records, done 2026-09-29, see "The
multiball in a run"; done 2026-09-29: the table
angle in a run and the countdown of `SERVE_SECONDS`, the ball save, see
"The table angle and the ball save in a run"). The game
     phase 0 of CODE:BAD6 done 2026-09-29 (see "Game phase 0"; 8 see "The
     player record in a run", 9 "The tilt in a run", the others "Esc"),
     so every phase is known. Done 2026-09-29: the SVGA modes in the
     runner (see "The SVGA modes in a run"; FRAME_RATE 59 or 60).
4. For the port (written 2026-09-29, at the user's suggestion): the
   original's slips as options, off by default, so that the default
   plays as the original does (scores and extra balls alike). Found so
   far, both through the shared cell CODE:0004 that host routines use
   as a work cell:
   - table 1's shooting game: the 25-hit extra ball (see "Table 1's
     shooting game when the lives run out"); the fix keeps the state
     pointer across host vector +18h, so byte +8 is read from the state;
   - the hole's sound record read from a stale [CODE:0004]+10h (the
     hint at CODE:30996; see "The hole's sound in a run": the original
     plays nothing there and skips that frame's draw, the fix plays the
     hole's own record 7 times an eject).
5. The port (stage 3), in steps (planned 2026-09-29):
   - done 2026-09-29: the port's memory in doskit (`pmem.h`: 16 MB of
     linear memory, the pMAX image loaded at a linear address with a
     selector per descriptor, 32-bit accessors, `pm_write`; the runner's
     `-mem`; `memcmp.py --base`; symmap.py with 32-bit offsets). The
     game's numbers: see "The image in memory at its entry".
   - done 2026-09-29: the port loads `ILLUSION.386` from the player's
     `ILLUSION.EXE` (port/src/archive.c, the decoder as tools/illfiles.py
     has it; the SHA-256 from port/src/gen/names.h) at 100F30h with
     selector 1Ch; its memory at ENTRY equal to the run's (port/README.md,
     "Checked"). Not looked at: whether pMAX takes a loose
     `ILLUSION.386` before the archive's (the loader opens one), which the
     port does not;
   - done 2026-09-29: ENTRY up to SETUP_ARGS in C (port/src/entry.c;
     the configuration file read as "The configuration file's options"
     says, port/src/pmax.c), the port stopping by name at the first
     routine not translated; memory equal to the run's at SETUP_ARGS.
   - done 2026-09-29: SETUP_ARGS (port/src/setup.c; SETSOUND.DAT
     through a pMAX heap of the port's, port/src/pmax.c): memory equal
     at SVGA_CHECK (see "SETUP_ARGS in a run"); the /S, /O and /R
     branches stop at SOUND_SETUP, OPTIONS_SCREEN and OPTIONS_RESET.
   - done 2026-09-29: SVGA_CHECK for the VGA resolutions (its SVGA
     branch stops the port); memory equal at VGA_INIT's first call
     (linear 1013CAh, t=2.425942: nothing changed since SVGA_CHECK). The
     order in ENTRY is SVGA_CHECK, IRQ 1 masked, VGA_INIT, then
     HISCORE_INIT (OPTIONS+7 is HISCORES, see the hints).
   - done 2026-09-29: VGA_INIT and HISCORE_INIT (port/src/video.c, on
     doskit's vga.c and frame.c): memory and video memory equal at
     CODE:757D (linear 1084ADh, t=2.641916, 15 frames in both). The
     "Loading" picture (LOADING_PIC) is on the screen then; its block
     lay at 1473C0h in the run, where the port's heap put it too (the
     second block seen: SETSOUND.DAT's place again, freed before). The
     DAC is not compared (memcmp.py has no DAC; the same 300h bytes go
     to it).
   - done 2026-09-29: CHOOSER_LOAD's start and SOUND_START's CD check
     (port/src/cd.c, sound.c: CD_INSTALLED, CD_LOCK, CD_READ_TOC and a
     MSCDEX of one data track in pMAX's real-mode buffer 0B3Eh, as dosrun
     without -cue); CODE equal at CODE:70BD (linear 107FEDh) against a run
     with NOSOUND.SDR in the header (build/pm/nosound.cfg made as in "The
     CD check and the driver's start in a run"). The real-mode buffer
     differs where pMAX itself put the header (its file I/O), not modelled.
   - chosen by the user 2026-09-29: a silent stand-in driver first, the
     translation of NOSOUND.SDR (src/NOSOUND.hints, rebuilt identical
     since doskit's `bin` kind), compared with NOSOUND runs; real sound
     later. Next: the driver loaded (INT 94h AH=1 by DRIVER_CFG's name),
     INT 93h AH=8's alias (0Ch), CALLBACKS_CS, then NOSOUND's command 0
     (CODE:09F0: CODE:10BB, 137A = host callback 0 of 800h bytes, 11C6 and
     1214 = the DMA buffer through callbacks 1, 3, 2, 053D = its own INT
     92h of 8202h bytes) and command 4 (CODE:1C28, the MOD loader).
   - pMAX's heap, needed for that (walked in -mem dumps with 10h-byte
     headers `01, used FFh/00, selector, size rounded to 16, name offset,
     name selector, policy`): a chain from 1473B0h to FEFFF0h, first fit
     from the bottom for policy 0; MOD.INT (host callback 6, INT 92h AH=8
     BL=1) cut from the top (data at FA8060h, header policy word 2); host
     callback 1's block (BL=2, selector 34h) not in the chain but at linear
     13120h, DOS memory (the trace after callback 3 at driver CODE:1258).
     Selectors: the lowest free of 04h, 0Ch, ... in steps of 8, with 14h,
     1Ch, 24h taken from the start; 48h (video) apart. port/src/pmax.c
     does only the bottom-up case so far.
   - on from HISCORE_INIT: CODE:757D,
     the chooser CODE:4CFB and the table CODE:A323 (ENTRY's loop), down to
     the main loop (GAME_PHASE's dispatch at CODE:BAD6); then the parts
     already read, each compared with memcmp.py;
   - stage 1 for `SOURCE\T001.BPC` before table 1's code is needed;
   - not read yet and wanted: the ball's physics and collisions
     (CODE:1352D, the map DROP_MASK writes, header slot 18), the
     flippers' movement, the drawing and scrolling; whether any of it
     uses the x87, which the runner does not emulate.

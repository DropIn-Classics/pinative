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
run"); pMAX's own unpacker read in a run's memory, and the archive's
decoder (tools/illfiles.py, port/src/archive.c) corrected after it: 10
of the 125 entries had come out wrong (see "pMAX's decoder"); the
chooser's retrace timer in the driver (see "The chooser's timer");
the chooser's backdrop and captions in the port (see "CHOOSER_WAIT's
loop"); the chooser's end in the port (see "The chooser's end"); its
Info and greetings pages (see "The Info and greetings pages").

## The earlier analysis

Kept: its notes (docs/*.md) and its data-format tools with their
synthetic tests, `tools/bpc_inspect.py`, `asset_inspect.py`,
`sdr_inspect.py`, `cfg_inspect.py` (`python3 -m unittest discover -s
tests`: 45 tests ok, Python 3.9; 52 with tests/test_event_streams.py,
Python 3.12 on Windows, 2026-09-29; 54 on macOS, Python 3.9, with the
drop-target banks' test, 2026-09-29). Run on `build/files/` 2026-09-29 they
accept all four tables (`DATA\S00n`, `SOURCE\T00n.BPC`/`.REL`) and every
driver but `SNDSCAPE.SDR` ("no EAX jump-table dispatch", the driver the
notes already call anomalous; accepted since the decoder's correction of
2026-09-29, see "pMAX's decoder"); `cfg_inspect.py` is not run, there is no
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
- doskit from 2026-09-30 on (merge b985c4d): the data folder is always
  the user's (`~/.local/share/pinative`, `%LOCALAPPDATA%\Pinball
  Illusions`, `~/Library/Application Support/Pinball Illusions`), never
  beside the program; the port moves a `game` folder from beside it
  there on its first start (`sys_data_migrate`), so `port/build/game`
  moves too. Checked on Linux with a copy and `DK_DATA_DIR`: moved,
  found, the run went to ENTRY. Not checked on Windows or the Mac.
- Comparing play (since 2026-10-01): give both the same keys by table
  frame: the port's DK_KEYS picture = frame + 5902 (all tables; the
  chooser's Enter at 5637 and 5815, Down at 5696, 5740, 5780), the run
  `-keysat 12A7F5 FILE` with lines "frame+1 KEY+" / "KEY-" (CODE:298C5's
  pass) and the chooser's `-key 112 enter -key 115 enter` (Down at 114;
  113, 114; 113, 113.5, 114 for tables 2, 3, 4); stop the port at a
  frame with a scratch check of FRAME_STEP's count after its wait and
  the run with `-break 12A7F5#frame`; compare CODE with memcmp.py and
  the heap 1473B0h..FF0000h by a small script (the driver's block at
  1473C0h and the free top differ by design). One key per frame only
  (see "The objects a ball hits in the port"). The scripts used lived
  in build/p7 (not kept).
- RELEASE.md point 7 (newer releases made known, asked once on the
  setup screen, `update.h`) is not in the port: the port has no setup
  screen of its own yet, the game's SETSOUND is not the place. The
  build scripts link `update.c` and pass `PORT_UPDATE_URL`, the
  workflow makes `latest.json`; the question and the notice are to be
  added before a first release, and README.txt's "New versions"
  section with them.
- doskit to 69d03a2 (2026-10-03, 16 commits since bd81e75): the setup
  screen as pddnative's (a menu with LI_PAGE windows, Esc to Quit),
  the dialog about the game's files every port shares, INT 33 mouse
  support, critical-error injection, xfer/symmap fixes. The port is
  rebuilt with it: its setup screen is the kit's menu with one page
  per group (Play actions, Skip the intro, Display, Game, Sound, Keys,
  Controller), its questions the kit's file dialogs with the copy's
  progress bar; checked headless with `-setup` (the menu, every page,
  a key given, Esc to Quit, a table started at once, both dialogs
  declined; port/README.md). No `carried over` block in src/, so
  xfer's shorter carried comments change nothing here.

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

Found 2026-09-30: a name given twice in the hints (FADE_PAL at CODE:6380
and again at CODE:A72B) passes disasm.py, symmap.py and check.py
without a word; symmap.py kept the second address, so the intro's
routines wrote to the table's palette. A check for it belongs in doskit
with a test (rule 7); not done. Until then: `awk '$1=="name"||$1=="code"
{print $3}' src/ILLUSION.hints | sort | uniq -d` before a new name.

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
  ask for that address size. Since doskit 346bbfc (2026-09-30) they are
  written `ES:[SMALL 27H]` (TASM's operator) and their 13 raw hints are
  gone; build.py IDENTICAL with the 4 below, 30,189 instructions;
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

## Stage 1: T001.BPC

Begun 2026-09-30 on Linux. A table module is relocated by its `.REL`
list (docs/bpc-module.md), so doskit got a way to read that list
(doskit d4b7ccd, acedd78: `bin ... noentry` and `offrel FILE
flags=MASK`): exactly the listed dwords are offsets, an instruction's
immediate or displacement or a DD in data, every other number stays a
number, and build.py checks that the rebuilt source's offsets are
exactly the list's. No guessing of offsets (ptrscan.py) is needed for
the modules.

`python3 doskit/tools/build.py src/T001.hints`: IDENTICAL, "offsets:
the 1707 of the offrel list, no other", 435 instructions, 693 labels,
1 line as DB, 1.8 s. gaps.py: 72,124 of 73,728 bytes not reached as
code yet. The hints: the header's four code slots (32, 39, 40, 41) as
`words` and entry points, named after docs/bpc-module.md; one `raw`.

- 133 instructions have a 16-bit address and no register (67h,
  `mov esi,[0x14]`): the module reads and writes the main program's
  cells at DS:0..3Fh (CODE:0010 the host vector, see "Stage 1:
  ILLUSION.386", CODE:2CD10; [14h] the state pointer,
  docs/bpc-module.md), so its DS is the main program's CODE (not checked
  in a run). The whole module will have many more, so doskit 346bbfc
  gave tasm.py TASM's `SMALL`: they are written `DS:[SMALL 14H]`, no raw
  hints (decided 2026-09-30).
- CODE:394C: a DS prefix where DS is the default, EBP the index, as the
  three in ILLUSION.386. Its displacement is an offset of the list; the
  line is written DB, DD label, DB (doskit acedd78).
- Slot 42 holds 0 and is in the `.REL` list in all four modules, so
  once loaded it points at the module's base; docs/bpc-module.md said
  null slots are not in the list. The one header slot not in a list is
  table 4's slot 30, which holds 0 (table 4 has no second music module,
  docs/bpc-module.md). Checked with a scratch script, 2026-09-30.
- The `.REL` lists are not sorted; the order is not looked at.
- 2026-09-30: with the opcode-14h object's two methods as entry points
  (SHOOT_START, SHOOT_UPDATE) build.py says IDENTICAL, 955
  instructions, 723 labels; gaps.py leaves 4 gaps, all data (the
  header, 0149h-37ADh, 3D5Ch-9A4Eh, A193h-end). So the module's code is
  three runs, 00B4h-0149h, 37ADh-3D5Ch, 9A4Eh-A193h, all reached. Of
  the relocated dwords outside them, exactly six point into them: the
  header's slots 32, 39, 40, 41 and the object's two methods (checked
  with a scratch script on T001.REL). The calls through the host vector
  counted in the rebuilt source: +04 3, +08 4, +0C 4, +10 3, +14 6,
  +18 2, +1C 3, +28 2 (27, three of them through `DS:[EBP+n]`), the
  earlier scanner's row for table 1 exactly (docs/bpc-module.md, "Host
  callback vector"). MOD_NEXT_BALL read: the bonus multiplier's
  counter 4502h and its lamps set again for the player (the hints).
- 2026-09-30, the other three modules, src/T002.hints .. T004.hints
  made the same way (the header's code slots, the opcode-14h objects of
  tables 2 and 4), each IDENTICAL: table 2 648 instructions, code at
  00B4h-0153h, 381Dh-3DBBh, 9A10h-9D88h; table 3 399, at 00B4h-00B7h
  and 370Dh-3CB4h (no object); table 4 1558, at 00B4h-00B7h,
  36D1h-3C05h, 97EDh-B1E3h. Table 4's sea game calls its routines
  through a register loaded with their offset (`MOV EDI,OFFSET`,
  `CALL EDI`), jumps through a table of relative words (CODE:9EBE) and
  keeps a routine pointer at the head of each of its two scripts;
  disasm.py follows none of these, so they are `code` hints (13). A
  search of all four rebuilt sources for a register loaded with a data
  label and then called or jumped to finds nothing more, and every
  relocated dword outside code that points into code is a header slot,
  an object's method or one of table 4's two scripts. Two raw lines
  more: the DS-prefix form of table 1 at table 3's CODE:38A4 and table
  4's CODE:3859, and table 4's `LEA EBX,[0152h]` with a 16-bit address
  (CODE:A989, a constant 152h). The routines named from
  docs/bpc-module.md, which had them all.
  The host-vector calls of tables 2, 3 and 4 counted as table 1's: 22,
  18 and 29, each the earlier scanner's row exactly; 11 of table 4's go
  through the sea game's copy of the vector (SEA_VECTOR, 9874h). The
  sea game's two sprite routines differ in where the offset SPRITE_SKIP
  goes: into the picture (the columns) or the display (the boat).

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

### The driver loaded, in a run

The run with NOSOUND.SDR in the header stopped at the driver's command 0
(CODE:711D, linear 10804Dh, t=2.748817; `-mem` build/pm/ns_711d.mem):
against CODE:70BD only the eleven selector words of HOST_CALLBACKS
(1Ch -> 14h), DRIVER_SEL (4) and DRIVER_ENTRY's selector (0Ch) change.
The driver's block is at 1473C0h behind the header `01 FF 04 00`, size
3500h, then 7075h and 1Ch (CODE:7075, "Sound Driver", the name INT 94h
AH=1 gets in ESI). The port (the lowest free selector, 14h, 1Ch, 24h and
48h taken) gives the same 4 and 0Ch; its memory is equal there, the
driver's block included (port/README.md, "Checked").

### The driver's command 0 in a run

The same run stopped after command 0 (CODE:7124, linear 108054h,
t=2.766845; `-intwatch 92`, `-mem` build/pm/ns_7124.mem). NOSOUND's
command 0 (names in src/NOSOUND.hints) keeps DS, ES:EBX and FS:EDI,
saves ports 61h, 21h and A1h (30h, BAh, FFh: dosrun's answers), then:

- CHAN_BUF_ALLOC: host callback 0, 800h bytes, selector 2Ch, header
  at 14A8C0h (after the driver's 3500h bytes), the data cleared; the
  selector also into +0Ah of four 3Bh-byte records at CODE:2C3A;
- BUFFERS_ALLOC: MIX_RATE AC44h (44100, from the file) / 50 = 372h,
  twice that 6E4h, times NBUF 3: A56h and a DMA buffer of 14ACh bytes;
- DMA_ALLOC: host callback 1 (INT 92h AH=8 BL=2 around AH=0Ah) gives
  selector 34h at linear 13120h, DOS memory, not in the heap's chain;
  it does not cross a 64 KB line, so no filler; cleared;
- VOLTAB_MAKE: 8202h bytes ("Volume table") at selector 3Ch, header at
  14B0D0h: 41h rows of 100h words, row v entry b = (signed b) * v.

So pMAX's selectors went 4, 0Ch, 2Ch, 34h, 3Ch: the lowest free, with
14h, 1Ch, 24h taken. The heap chain after it: 1532F0h, the free rest
to FEFFF0h. The port (port/src/nosound.c, hostcb.c) leaves the image,
the driver's block and the three new blocks equal; what else differs in
the whole memory is pMAX's own (below the image, the block headers, and
bytes near the top of memory the run left there before the image was
loaded, presumably pMAX's unpacking; not looked into).

### The driver's command 4 in a run

The run with NOSOUND.SDR in the header (build/pm/nosound.cfg by `-put`,
no `-cue`) stopped at the end of SOUND_START (CODE:75B6, linear
1084E6h; `-mem`), against the port stopped there (`-cfg
build/pm/nosound.cfg -mem`), 2026-09-29, Linux:

- command 4 (CMD_LOAD_MODULE, then MOD_LOAD; names in src/NOSOUND.hints)
  gets `intro\MOD.INT` (INTRO_MOD_NAME) through host callback 6: pMAX
  loads the whole file (47F88h bytes, selector 44h) from the top of the
  heap, its data at FA8060h so that the size rounded to 16 ends at
  FEFFF0h; the header's last word there is 2 though callback 6 asks for
  policy 1 (INT 92h AH=8 BL=1), not looked into. Callback 6 leaves BX
  (MOD_HANDLE is 80BCh, the name's offset);
- "M.K." at 438h: 31 samples; NPATTERNS 2Ah (the highest entry of all
  80h orders, + 1), SONG_LENGTH 37h, RESTART_POS 0; the patterns (A800h
  bytes, selector 4Ch) and 28 sample blocks (54h to 12Ch, each its
  length + 800h bytes, the loop repeated after it) cut from the bottom,
  first fit, after the volume table; MOD.INT freed at the end (selector
  44h free again). Each note is rewritten by NOTE_CONVERT (the period's
  index in PERIODS + 1, the sample, the effect, its parameter);
- selectors: 48h, the video memory's, is not one of the local table's
  (bit 2 clear) and does not keep 4Ch from being given: the port had
  marked it taken and gave 54h for the patterns until this was seen;
- equal in the port: CODE and TAIL (memcmp.py src/ILLUSION.hints ...
  --base 100F30), the driver's block (src/NOSOUND.hints, --base
  1473C0), all 32 blocks of the heap's chain, the DMA buffer at 13120h
  and MOD.INT's freed bytes at FA8060h. Only once MOD.INT itself was
  equal (see "pMAX's decoder").

The result of command 4 is not looked at by SOUND_START (CLC after the
call); SOUND_JUMP (CODE:1550) set to 4CF2h at its end.

### pMAX's decoder

The first comparison above showed the samples wrong from 42Eh into the
first one: the run's MOD.INT in memory differs from the file
tools/illfiles.py (and port/src/archive.c, which was checked only
against it) unpacks from byte B06Ah on, where ours turns to noise and
the run's goes on as a waveform. B06Ah is 29 codes after the LZW
table's first reset (at output B04Dh); the symbol there came out one too
high.

pMAX's unpacker is not in ILLUSION.EXE's bytes as stored: the loader
(the MZ image, at linear 760h in the run, segment 76h) is equal to the
file only in its first F4h bytes; 18836 of its 21264 bytes differ, so it
decrypts or unpacks itself (not followed). In the run's memory
(build/pm/ns_75b6_new.mem, not in the repository) it is 16-bit code with
32-bit operands; offsets in segment 76h (the variables' DS taken to be
76h too, presumably):

- 0076:378A the unpacking: 340E (coder state), 3447 (INT 21h AX=4200h
  to the entry, then INT 91h AH=5 with ECX 7D00h, presumably a read of
  that many bytes; 373A does it again when they are used up), 33C6
  (the table: 8-byte entries, next code and alphabet 100h), 3783 (skips
  4 bytes, presumably the entry's second size dword), 3715 (code and
  the bit buffer from four big-endian words); per code 34ED (the symbol), 3475 (the new
  interval), 3552 (normalization), then the alphabet + 1; the table
  full at 1F3Fh: 33C6 again (38AF);
- variables: 335D high, 3361 low, 3365 code, 3369 the alphabet's size,
  336D the next code, 3379 the bit buffer, 337D its count of bits used
  (a byte), 3381 the pending underflow bits.

Where ours differed, all in the normalization (3552): with leading bits
of low and high alike and pending bits, pMAX shifts one bit, clears the
pending count and looks again (so it may take an underflow step in the
same call); ours shifted all alike bits and kept the pending count until
a call with no bit alike. And an underflow step that crosses the bit
buffer's end refills twice and does not refill again at 16. The
dictionary, the symbol and the interval arithmetic were the same.
BSR with a zero source leaves its destination (as dosrun does); pMAX's
ECX carries over there, and the new decoders keep that too, unseen in
these files.

Written after it (tools/illfiles.py's Decoder, port/src/archive.c): the
unpacked MOD.INT equal to the run's bytes at FA8060h, ILLUSION.386 still
with its SHA-256, and archive.c equal to illfiles.py for all 125
entries (a scratch program). Ten entries come out other than before (the
first differing byte): `CHOOSER\FILE1.DAT` 6C80Fh, `INTRO\MOD.INT`
B06Ah, `INTRO\INTROPIX.MGL` 45847h, `DATA\S001\FILE3.DAT` 462F9h,
`SOURCE\FILE8.DAT` F2DC1h, `DATA\S002\FILE4.DAT` 1BA88Bh,
`DATA\S003\MUSIC2.MOD` 19619h, `DATA\S003\FILE5.DAT` 3A8ECh,
`DATA\S004\FILE6.DAT` 1DAD03h, `SNDSCAPE.SDR` 1D5h. So `build/files/`
must be unpacked again (`extract --all`); what was read from those ten
before (the earlier notes on SNDSCAPE.SDR, the FILEn.DAT sizes'
remarks, table 3's second music) wants a second look.
tools/sdr_inspect.py accepts SNDSCAPE.SDR now (the dispatcher at 1258h,
the table at 12AEh), as the other eleven. Past the archive's end the
decoders read zeros; pMAX's word read gives something else there (its
buffer position), not checked, as no entry seen needs it.

### The chooser's files in a run

The run with NOSOUND.SDR in the header (build/pm/nosound.cfg by `-put`,
no `-cue`) stopped after CHOOSER_LOAD's eleven INT 94h AH=1 (CODE:7885,
linear 1087B5h, t=13.946465; `-mem` build/pm/ns_7885.mem), 2026-09-29,
Linux. CHOOSER_LOADED (CODE:5DCF) set to 1 first, then the files in
this order, each with an empty block name (the 0 before each file name
in CODE), their selectors stored by the POPs after each call (names in
the hints): chooser\cube.rix 44h (MOD.INT's, freed by command 4),
tube.rix 134h, torus.rix 13Ch, tinyfont.fnt 144h, infodata.mgl 14Ch,
menuchar.rix 154h, intro\introani.roy 15Ch, intropix.mgl 164h,
SCROLL.DLT 16Ch, BKGR.FLD 174h, PCSKY.FLD 17Ch. For introani.roy EDX
is kept too (INTROANI_SIZE, 157FCh: the file's 88060 bytes), so INT
94h AH=1 gives the file's size in EDX; INTROANI_POS set 0.

pMAX's heap: the blocks first fit from the bottom after the 32 of "The
driver's command 4 in a run" (cube.rix's header at 1A90B0h, PCSKY.FLD's
at 29CD50h), the free rest from 2AFC60h to FEFFF0h one block: MOD.INT's
freed block at the top merged into it. The port (port/src/intro.c)
leaves CODE and TAIL and all 43 used blocks of the chain equal
(port/README.md, "Checked"). The next call is CODE:7341, through the
checksummed jump CODE:7438 ([CODE:904F] less the byte sum of CODE:7230..
7ABB; computed from the run's memory at CODE:75B6). The targets of the
other checksummed calls in CHOOSER_LOAD, computed the same way and not
yet seen in a run: [CODE:908B] 698Ah, [CODE:9097] 6FC0h, [CODE:9043]
6E49h, [CODE:907F] 72BBh, [CODE:235E] 73D8h (the tail jump at the end).

CODE:7341 (INTRO_PALS_MAKE; names in the hints) makes INTRO_PALS, 16
rows of 16 DAC colours, from PCSKY.FLD's first 30h bytes with a value
added to each (at most 3Fh), row 3 cleared; its EDI comes from CODE:7441,
the same checksum trick ([CODE:905B] less the byte sum of CODE:6F43..
7028: 8D3Ch). Then CHOOSER_LOAD sets the intro's script: INTRO_SCRIPT
(CODE:7ABC, 192 entries of a position and a routine) is run by
INTRO_TICK (CODE:7029) against the driver's command 0Dh, presumably the
module's position (not checked); the last entry (position B4Eh) is
CODE:6E1E, which sets INTRO_END, what the loop at CODE:7996 waits for.
The same run stopped at CODE:78D9 (linear 108809h, t=13.949067; `-mem`
build/pm/ns_78d9.mem): the port equal there too (CODE, TAIL, the 43
blocks).

The next two checksummed calls (CODE:78C9, 78DE) go to CODE:698A
(INTRO_MODE: mode 13h unchained, the intro's CRTC values, all four
planes cleared) and CODE:6FC0 (INTROPIX_PALS: INTRO_PAL1 and INTRO_PAL2
from intropix.mgl's 32 colours of 12 bits; its EDI 813Ch by CODE:7441,
[CODE:9067] less the byte sum of CODE:698A..7ABB). Then FADE_FROM and
FADE_TO are set to INTRO_BLACK (CODE:873C, zero then), FADE_LEVEL 40h,
FADE_STEP 4, the start address 2D50h, and INTRO_FRAME (CODE:6E35) is
called: the driver's command 6, then FADE_FRAME. The run stopped at
that call (CODE:7925, linear 108855h, t=13.963461; `-mem`
build/pm/ns_7925.mem, `-vram`): the port equal there (CODE, TAIL,
video memory); `-vgastate` there shows the registers INTRO_MODE writes
(misc E3h, sequencer 4 06h, CR 11h 2Ch, start 2D50h), the port's
registers not compared.

### The driver's player

NOSOUND.SDR's command 6 (CMD_MIX) is the module player; its effect
handlers were reached only through tables and pointers the code stores,
so they were DB in the source: the hints now name the tables (ROW_FX,
ROW_FX_SFX, ROW_FX_E, ROW_FX_E_SFX: 16 dwords each; a `words` count is
hex), the tick routines a channel's dword +0 holds (`ptr` at each
store), the mixer's MIX_ROUTINES (CODE:00C0, stride 8) and its two
unrolled loops with their entry tables, and TIMER_IRQ (CODE:0658, set
through host callback 1Eh). The whole driver is code now but for its
data (build.py IDENTICAL, 2210 instructions).

What the code says (names in src/NOSOUND.hints): MIX_UPDATE mixes into
the DMA buffer at MIX_POS, FRAME_BYTES (MIX_RATE / 50) a call before
command 1 (PLAYING 0), after it as far as TIMER_IRQ's SAMPLE_POS (IRQ 0
at MIX_RATE: PIT divisor 1234DCh / MIX_RATE, 27), in pieces ending at a
tick's line; at each line TICKS is counted and the module's tick played
(TICK_FX, or ROW_PLAY every SPEED ticks). ORDER_POS (named ROW_OFFSET
before) is the position in ORDERS, PAT_OFFSET the next row's offset
(pattern * 400h + row * 10h). Command 0Dh returns TICKS less the ticks
still in the buffer: the intro's script (INTRO_TICK) runs on the
module's 50 Hz ticks. A channel's +37h 1 marks a jingle's channel, whose
end gives the channel the music's state back (C2D26..).

The run stopped at the intro's command 1 (CODE:795F, linear 10888Fh,
t=13.983528; `-mem`, `-vram` build/pm/ns_795f.*), after one INTRO_FRAME
(one command 6: the module's first row and one tick mixed), the screen
filled with 3Fh, the overscan 3Fh and INTROPIX_SHOW: the port
(port/src/nsplay.c) equal there: CODE, TAIL, video memory, the driver's
block, the DMA buffer at 13120h (1333 bytes not zero) and the 43 used
heap blocks. Only that row's effects ran; the other handlers are not
checked against a run yet.

### The driver's timer

Command 1 (CMD_PLAY, CODE:0B31; names in src/NOSOUND.hints) sets NBUF
from CX (the intro asks for 0Fh; NBUF_SET makes the DMA buffer again
for another count), mixes NBUF frames ahead and starts the timer
(TIMER_START): IRQ 0's vector by host callback 4 (kept at OLD_IRQ0, in
the run 0030:0000414B) set to TIMER_IRQ by callback 5, the PIT's
channel 0 in mode 3 with 1234DCh / MIX_RATE (PIT_DIV 1Bh: IRQ 0 at
about 44192 Hz). TIMER_IRQ counts SAMPLE_POS, the place in the DMA
buffer a card would be playing, which MIX_UPDATE mixes up to. Command
0Dh (CMD_POSITION) returns TICKS less the ticks still unplayed in the
buffer; INTRO_TICK runs INTRO_SCRIPT on it.

The port has no interrupts: src/nosound.c counts the IRQs of one
picture (1193182 / PIT_DIV / vga_refresh_hz()) at once, from frame.c's
tick; host callbacks 4 and 5 only answer for IRQ 0 (the vector dosrun
gave) and keep nothing. So SAMPLE_POS moves in steps of a picture, not
of a sample.

The run (NOSOUND.SDR in the header, build/pm/nosound.cfg by `-put`, no
`-cue`, `ILLUSION.EXE C:\ILLUSION.CFG /`) stopped at the intro's end
(CODE:79C3, linear 1088F3h, t=72.095299, 3480 frames, no key pressed;
`-mem`, `-vram` build/pm/ns_79c3.*), 2026-09-30, Linux, against the
port stopped there: CODE and TAIL 0 bytes differ (INTRO_TIME,
INTRO_NEXT and INTRO_END among them), video memory equal, 42 of the 43
used heap blocks equal, the DMA buffer at 13120h equal. The driver's
block differs in 24 bytes: SAMPLE_POS, TIMER_COUNT, MIX_POS, MIX_LEN
and each channel's position and fraction (+4..+7), how far the mixing
stands in the sample clock, which the port counts by pictures. Not
checked: the keys that leave the intro (Esc, space: port 60h read with
IRQ 1 masked; Enter is not one, the MOV AL,1Ch before the third JE sets
no flags), the DAC, the SDL build's pictures by eye.

### The intro's end

After INTRO_END (CODE:79C3; names in src/ILLUSION.hints): a fade from
CODE:8A3C to INTRO_PALS, VIEW_2D50, then through CODE:4CF2's checksummed
jump ([CODE:907F] less the byte sum of CODE:6A20..8115) INTRO_BKGR at
CODE:72BB: BKGR.FLD and PCSKY.FLD combined to 3CF0h (a PCSKY dword ORed
with the BKGR dword shifted left 4, so BKGR gives pixel bits 4-7 and
PCSKY 0-3, presumably; not looked at as pictures). Then SCROLL_FRAME
(CODE:7230) once a frame: SCROLL.DLT's next 240 lines XORed into pixel
bit 7 (write mode 2, function XOR, bit mask 80h), SCROLL_POS moved on by
one line each frame, while SCROLL_LEFT counts 780h down to 21h (1887
frames), then a fade to INTRO_BLACK (32 frames, the scroller going on).
Esc or space in the script go straight to CODE:7A82, in the scroller to
the fade. CODE:7A82: IRQ 1 unmasked, the driver's command 3 (CMD_STOP:
TIMER_STOP, CHANNELS_RESET, the ports back) and command 8 (CMD_ORDER,
slot 0 from order 12h, the music for the chooser presumably; not played
until another command 1), then through CODE:7438's checksummed jump
([CODE:235E] less the byte sum of CODE:4CE8..55E2) INTRO_FREE at
CODE:73D8, code the hints had as bytes: INT 92h AH=5 for the five intro
blocks. Its RET goes to CHOOSER_LOAD's, and ENTRY calls the chooser,
CODE:4CFB. The two jump targets were computed from the run's memory and
seen reached in runs (CODE:72BB at t=72.13, CODE:73D8 at t=104.36).

CMD_PLAY and CMD_MIX end at CODE:0975, whose POPAD gives the caller its
own EAX back; the port had set EAX from RESULT there (no caller reads
it), now left as it came.

Runs 2026-09-30, Linux (NOSOUND.SDR in the header as in "The driver's
timer"; `-break 105C2B`, CODE:4CFB), against the port stopped there:

- no key (t=104.362785, 5401 frames; build/pm/ns_4cfb.*): CODE, TAIL
  and video memory 0 bytes differ; 37 of the 38 used heap blocks equal,
  the five freed ones merged with the free top of the chain in the
  original (the port's allocator looks only at used blocks: the same
  gap); the driver's block differs in the sample clock only (SAMPLE_POS,
  TIMER_COUNT, MIX_POS, MIX_LEN, the channels' positions, TICKS by 1);
- `-key 40 space` (in the script; CODE:4CFB at t=40.016607) against the
  port with DK_KEYS 1540 (space at picture 1540): CODE differs only in
  INTRO_TIME (5 09h against 4 F3h ticks: the key at another moment),
  heap as above;
- `-key 90 space` (in the scroller; t=90.554041) against DK_KEYS 4550:
  CODE differs only in SCROLL_POS and SCROLL_LEFT (38h against 30h),
  heap as above.

The keyboard after CODE:7A82 (IRQ 1 unmasked: which INT 9 handler takes
the keys then) is not followed; the port hands the keys to nobody.

### The chooser's start

CODE:4CFB to CHOOSER (CODE:4FF9) is a chain of checksummed jumps and
calls (each target a dword less the byte sum of some code), 16 of them;
the targets were computed from the run's memory at CODE:4CFB and the
path followed in a `-trace` of the run from there (3 million
instructions, `-log 105C2B -trace FILE 3000000`; the whole start takes
21 million, no frame in between: CHOOSER at t=107.847899). In order
(names in src/ILLUSION.hints): KBD_INSTALL (IRQ 1's vector by INT 93h
AH=3 BL=1, 0030:00004152 in the run, kept at KBD_OLD; CODE:340A set by
AH=4; AH=9 BL=1 not looked into), INT 10h mode 0Dh, CHOOSER_MODE (a
304-pixel mode of 2Eh bytes a line over 0Dh's registers),
ATTRACT_START, CHOOSER_DAC, SPLIT_SET (CX 1BDh through the XCHG [ESP]
trick at CODE:4D8B), CHOOSER_VIDEO, VIDEO_CLEAR, SCROLL_INIT,
MENUCHAR_INIT (menuchar.rix already loaded by CHOOSER_LOAD), CODE:28B8,
the overscan colour, WRITE_MODE1, READ_MODE1, VIDEO_TOP_SET,
BITMAP_ALLOC, CAPTIONS_MAKE, BITMAP_FREE; with CHOOSER_LOADED 1 the
files are not loaded again.

pMAX services new here (from the trace's registers and the heap in
`-mem` dumps): INT 92h AH=7 cuts a block from the top of the heap with
a selector (5C08h bytes, selector 15Ch, the lowest free: the intro's
first); AH=6 cuts one below it and gives only its linear address in EAX
(10000h bytes at FDA3D0h), AH=2 frees that; INT 93h AH=0Ch gives
selector 80h, flat presumably (the chooser writes GEN_BUF through it by
linear address); INT 93h AH=14h (CX 9Ah: a code segment) and AH=9 leave
nothing the port models. Whether AH=7 always takes the top whatever the
policy is not known (seen once).

The captions are compiled. CAPTION_BUILD draws a caption's layout (text
of menuchar.rix, lines) into two bitmaps of 2E04h bytes, a bit a pixel,
then makes six 16-bit routines of them (GEN_CODE, GEN_CODE2) in
GEN_BUF, each copied into a block of its own (INT 92h AH=4, first fit
from the bottom: 138 blocks, selectors 164h to 5ACh); the record gets
their far pointers. The routines are made of: MOV SI,ES:[150Ch+2n]
and ADD SI,ES:[151Ch+2n] (every 32 lines, after ADD DI,5C0h), a latch
read MOV AL,[SI+d], a byte write MOV BYTE [DI+d],imm or MOV [DI+d],BH,
and a 32-bit RETF (66h CBh). The chooser draws a caption by calling
them (not followed yet); the port can draw by those few patterns
instead of running x86 code. CAPTION_BUILD's layout commands leave EAX
in states the next command reads the upper bits of (command 2's SHR
EAX,3 after a LODSW); the port tracks EAX as the code leaves it.

doskit's runtime had no mode 0Dh (vga_set_mode gave mode 13h's
registers for any other mode): added in doskit e9c69cf with the
runner's tables and a test.

The run (as in "The driver's timer", `-break 105F29`) against the port
stopped at CHOOSER, 2026-09-30, Linux: CODE, TAIL and video memory 0
bytes differ; 175 of the 176 used heap blocks equal (the 138 generated
routines among them), the driver's differing in the sample clock only
as at CODE:4CFB. The DAC and the CRTC's registers are not compared
(memcmp.py has neither); nothing is shown on the screen here (the
attribute controller's palette access is off until the overscan write,
and no frame is waited for).

### The chooser's timer

CHOOSER (CODE:4FF9) makes four checksummed calls before CHOOSER_WAIT;
their targets, computed from the run's memory at CODE:4FF9 (the dword
less the byte sum, as in "The chooser's start"): CODE:26FD, 4CB6, 236A
(through CODE:4CE8, called) and 38C6 (through CODE:4CF2, jumped to;
its RET goes to CODE:505C). Names in src/ILLUSION.hints and
src/NOSOUND.hints:

- CUBE_DRAW (CODE:26FD, AX CUBE_SEL): cube.rix from its byte 3Ah into
  video memory 72A4h, each plane (map mask 1, 2, 4, 8) four times,
  rotated left by 0, 2, 4 and 6 bits (a 16-bit ROL of a byte and the
  next; a line's last byte with its first), 32 lines of B8h bytes each
  time; the picture's lines are 2E0h bytes, B8h a plane. The map mask is
  left at 10h. Presumably the backdrop's shapes at four sub-byte
  positions for the scroll; not looked at as a picture.
- VSYNC_START (CODE:4CB6): the driver's command 0Eh with CS:4C85h
  (VSYNC_CB: VSYNC_COUNT, CODE:1548, counted) and 0Fh with CS:4C93h and
  ECX 1999h (CRT_START_CB: while CRT_START_ON, CODE:191A, is 1, CRTC
  0Ch/0Dh from CRT_START, CODE:1323). CS is 14h (DS 1Ch): the far
  pointers the driver keeps hold 14h.
- MUSIC_PLAY (CODE:236A): command 1 with ECX 0Fh.
- then CHOOSER_LOADED 0 and CHOOSER_WAIT.

NOSOUND's commands 0Eh and 0Fh (CMD_VSYNC, CMD_VSYNC2): command 0Eh
measures a picture with the PIT (FRAME_MEASURE, CODE:07D6: channel 0 in
mode 0 counting down from 0 from a retrace's start; the count at the
first read with the display on after the retrace, at the last such read
before the next retrace, and at that retrace). The run's numbers
(2026-09-30, Linux, as in "The driver's timer", `-break 1047F6`,
CODE:38C6, t=108.021548, 5403 frames; build/pm/ns_38c6.*): VS_RETRACE
BDh (189), VS_DISPLAY 4D76h (19830), VS_FRAME 4E35h (20021),
VS_FRAME97 4BDCh, VS2_TICKS 87Bh, VS_IRQS 2CEh, VS2_IRQS 50h, MIX_RATE
44100. They fit dosrun's CRT timing for the chooser's mode (its
vga_timing in doskit/tools/run/vga.c): CHOOSER_MODE leaves CR 0 at
mode 0Dh's 2Dh (its table starts at CR 1), so 50 characters of 8 dots
at 25.175 MHz / 2 a line, 528 lines (CR 6 0Eh, CR 7 3Eh), 20019.86 PIT
ticks a picture (59.6 Hz), and 5 retrace lines (CR 10h D7h with bit 8,
CR 11h's 0Ch) 189.58 ticks. So VS_RETRACE and VS_RETRACE + VS_DISPLAY
(20019) are the whole ticks of those, and VS_FRAME is two ticks more:
fitted to this run (the polling's delay, presumably), not derived. On a
real card the numbers are its own.

VSYNC_TICK (CODE:08A4) is TIMER_IRQ's at TIMER_COUNT 0 with VSYNC_ON:
it toggles VSYNC_PHASE; to 1 it waits (in the handler, interrupts on,
the IRQs still counted) for the retrace, sets TIMER_COUNT VS2_IRQS and
calls VSYNC_CB; to 0 it sets TIMER_COUNT VS_IRQS - VS2_IRQS and calls
VSYNC2_CB. CMD_PLAY starts TIMER_COUNT at VS_IRQS. So VSYNC_CB comes at
each retrace and VSYNC2_CB 50h IRQs (87Bh ticks, a tenth of the display
past the retrace's end) after it; the wait takes up the 3 % of a
picture VS_FRAME97 leaves.

The port (port/src/nosound.c) computes FRAME_MEASURE's numbers from the
CRTC's registers the same way (the two fitted ticks included) and waits
two pictures; TIMER_IRQ's picture of IRQs runs VSYNC_TICK where
TIMER_COUNT reaches 0, and a retrace wait ends at the next picture's
tick. The callbacks are the port's C (ns_far_call in chooser.c; any
other pointer stops the port). In the port both come in the tick before
the game's loop goes on; in the original VSYNC2_CB comes 87Bh ticks
after the retrace, so a CRT_START the game writes within that time is
shown a picture earlier there. Not seen in a run yet.

Against the port stopped at CHOOSER_WAIT, 2026-09-30, Linux: CODE, TAIL
and video memory 0 bytes differ; 175 of 176 used heap blocks equal; the
driver's block differs in the sample clock only (SAMPLE_POS,
TIMER_COUNT, TICK_COUNT, TICKS, MIX_POS, MIX_LEN, the channels'
positions), the timing numbers above equal. The DMA buffer at 13120h
differs (9446 bytes): CMD_PLAY mixes as far as the SAMPLE_POS left from
the intro's timer (MIX_POS 2470h in the run, 20CCh in the port, each
its SAMPLE_POS at CODE:4FF9), the sample-clock difference of "The
intro's end". The run's CPU time (CUBE_DRAW's writes: CHOOSER at
t=107.847899, CHOOSER_WAIT at 108.021548) is not modelled; the port
waits only FRAME_MEASURE's two pictures.

### CHOOSER_WAIT's loop

CHOOSER_WAIT (CODE:38C6; names in src/ILLUSION.hints, the block "CHOOSER_WAIT's
loop") runs once a retrace: FRAME_WAIT (VSYNC_COUNT to change),
CHOOSER_DAC, CRT_START_PICK, READ_MODE1, CAPTION_DRAW, ATTRACT_STEP,
WRITE_MODE1, LAG_SHIFT, SCROLL_STEP, PAL_FADE, CODE:23A2 (only
registers), MENU_KEYS, CAPTION_STEP, MUSIC_MIX, and KEY_HISTORY checked;
until MENU_DONE or CAPTION_OUT 4. How the backdrop moves, as read:

- The screen's start address moves on by 32 lines (5C0h) a frame, every
  third frame by 5C4h (32 pixels right too), wrapping at 44A4h back by
  3644h (SCROLL_NEXT, SCROLL_WRAP); CRT_START_CB shows SCROLL_TOP a
  tenth of the display after the retrace.
- Each frame STRIP_DRAW copies one band of 32 lines by 2Eh bytes by the
  latches (write mode 1) from CUBE_DRAW's picture into the band at
  SCROLL_TOP + 2840h and again C9BCh further (STRIP_COPY, unrolled):
  the column from SCROLL_COL and WAVE_TAB (a sine of 300h bytes, 80h to
  DFh), the rotated copy from its low 3 bits (SHIFT_OFFS).
- The caption is drawn at SCROLL_SHOWN by its compiled routines (the
  latches read from the backdrop, ORed with the caption's bits: READ_MODE1
  sets function OR); each 32-line band takes its SI from LAG_COL and
  LAG_PIC, which LAG_SHIFT moves one band on each frame, so the caption's
  backdrop follows the scroll band by band.
- ATTRACT_STEP: every 300 frames the next of nine palettes (CODE:191B,
  33h bytes each) faded in by PAL_FADE; after stage 9 the next backdrop
  (CUBE_SEL, TUBE_SEL, TORUS_SEL) drawn by CUBE_DRAW_WAIT over 32 frames
  (a frame every 16 lines, with CAPTION_DRAW2, the caption routines
  without latch reads) plus one.

The port (port/src/chooser.c) runs the caption routines by reading
their bytes (CAPTION_RUN: only the instructions GEN_CODE and GEN_CODE2
write; any other stops the port). FRAME_SPINS (CODE:154C), FRAME_WAIT's
count of its own polling, is read by no instruction and not kept by
the port. The path with CODE:1534 1 (set at CODE:4FF1, when the files
are loaded again) stops the port.

Runs 2026-09-30, Linux (NOSOUND.SDR as in "The driver's timer"), the
port stopped in FRAME_WAIT by DK_FRAMES (5401 pictures before the
loop's first FRAME_WAIT returns: DK_FRAMES 5500 stops it after 99
frames of the loop):

- the 100th CODE:38FD (dosrun `-break 10482D#100`, t=109.700627, 5502
  frames) against DK_FRAMES 5500: CODE differs only in FRAME_SPINS,
  TAIL and video memory equal;
- the 3000th (t=158.912646, 8435 frames: 2900 loops and the 33 frames
  of the backdrop change to TUBE_SEL, at about t=149 by the frame
  counts, not looked for in the run; BACKDROP 2 there) against DK_FRAMES
  8433 (3032 frames of the port's): the same; 175 of 176 heap blocks
  equal, the driver's block in the sample clock only (VSYNC_PHASE among
  it);
- Enter and Esc at t=112 (`-key 112 enter`, `esc`; CHOOSER_WAIT left,
  `-break 105F8C`, CODE:505C at t=112.653698 and 113.106981) against
  DK_KEYS "5637:1C 5646:9C" and "5637:01 5646:81": CODE only
  FRAME_SPINS, video memory equal. The key at picture 5637 was found by
  trying: Esc's fade (64 frames at CH_FADE_STEP 1) put it 7 pictures
  after 5630, where Enter had already matched (Enter ends CHOOSER_WAIT
  at a scroll wrap, so a window of pictures gives the same memory).

The headless port's pictures at 5700 and 7300 looked at: the cubes in
their wave with "Pinball Illusions", then the purple stage. Not
checked: the arrows and F1..F4 in CHOOSER_WAIT (they only set bytes
there), KEY_HISTORY's greetings path, the window build.

The caption flickered in the window (the user, Windows, 2026-10-01; seen
in the headless pictures 5600..5605 too: every other one without the
caption, a white line at the right). Why, as read from the loop above:
CRT_START_CB writes the start address a tenth of the display after the
retrace, which a VGA takes only at the next retrace; and the caption is
drawn into the page on show after the retrace, ahead of the beam. doskit
scanned the picture out at the tick, after the callback's write and
before the caption. Since 2026-10-01 the port turns on doskit's
`frame_set_scanout_end` (the start address latched at the retrace, the
picture scanned out at the frame's end): the headless pictures 5600..5603
all show the caption and no line; the intro (1500) and table 1 (6600)
looked at, as before. The white line's cause not looked into further (it
went with the change). Not compared with a run's pictures; the window
build not tried.

### The chooser's end

From CODE:505C (names in src/ILLUSION.hints, "the chooser's end"): with
MENU_DONE (Esc in CHOOSER_WAIT) straight to CODE:50A9; else MENU_ROW 0,
GREETINGS_PAGE when KEY_HISTORY asks, then MENU_LOOP (CHOOSER_WAIT's
loop with MENU_CAPTION for CAPTION_STEP, until MENU_DONE) and INFO_PAGE
while MENU_INFO is set and Esc was not pressed. CODE:50A9: without Esc
the keyboard table (INT 93h AH=15h gave AX 0 in the run, 2026-09-30,
Linux, the break at CODE:50FE: the QWERTY one at CODE:5E21) copied to
KEY_CHARS; MUSIC_STOP (the driver's command 3, now with VSYNC_OFF: both
retrace callbacks to the driver's RETF at CODE:0798 of its own code
selector); with Esc video memory "cleared" (map mask 0Fh, GC 5 0; GC 3
is still READ_MODE1's 10h, function OR, so each byte gets the latches)
and INT 10h mode 3. Then the driver's commands 5 (slot 0's module freed:
its samples longer than 2 and its patterns, host callback 2; MOD_FREE,
CODE:1B9F) and 0Bh (the driver's end: the DMA buffer, the volume table
and the channels' buffer freed), INT 93h AH=0Dh on DRIVER_ENTRY's
selector (the driver's code alias), CD_STOP (MSCDEX 85h), CHOOSER_FREE
(every caption routine's block, the chooser's six files and the
driver), KBD_RESTORE; AL MENU_ROW, FFh after Esc. ENTRY (CODE:033F):
AL below 4 is the table (AL + 1 into CODE:A323 after VGA_INIT), else
CD_LOCK 0, IRQ 1 unmasked and "Thank you for playing Pinball Illusions
CD." (CODE:07B4) before the RETF to pMAX.

Runs 2026-09-30, Linux (NOSOUND.SDR as in "The driver's timer"),
against the port with the keys at the pictures that matched in
"CHOOSER_WAIT's loop" (Enter at t=112 is picture 5637; t=114 was put at
5696, t=115 at 5815: 5816 left the state one frame behind):

- Enter at 112 and 115 (table 1), `-break 10B253` (CODE:A323,
  t=115.773853, AX 0401h): CODE only FRAME_SPINS, TAIL and video memory
  equal; the run's heap chain is one free block (EA8C30h bytes from
  1473B0h) and the port's allocator has no block left either (a
  temporary print), only the selectors 14h, 1Ch, 24h.
- Enter at 112, Down at 114, Enter at 115 (table 2, AX 0402h): the same.
- Esc at 112, the program's end (`-break 1012A1`, CODE:0371,
  t=113.116099): the same text on the console; CODE only FRAME_SPINS,
  TAIL equal. Video memory differs (198608 bytes): the run's BIOS set
  text mode 3, which the port does not; against the run before that INT
  10h (CODE:51D4) the port's video memory is equal.

Not checked: the pages (INFO_PAGE, GREETINGS_PAGE stop the port), F1..F4
and Up in the menu, a country other than 0.

### The Info and greetings pages

INFO_PAGE (CODE:4504) and GREETINGS_PAGE (CODE:4886); names in
src/ILLUSION.hints, the block "the Info and greetings pages". Both go
through PAGE_MODE: video memory cleared over 8 frames, the DAC's first
128 colours from CODE:8A3C, CRT_START_ON 0, then a 256-colour mode with
chain 4 off (4Ch bytes a line, 304 pixels; its CRTC from PAGE_CRTC,
59.6 Hz in dosrun as the chooser's mode). A frame of colour 1
(PAGE_BORDER), a palette faded in by PAGE_FADE (128 colours, sent each
frame by PAGE_DAC), then once a frame until Esc (PAGE_KEYS):

- INFO_PAGE: the page of MENU_ROW in INFODATA.MGL (PAGE_BASE, the dword
  at 4 + 4 * MENU_ROW): at +0Ah the palette (300h bytes), at +30Ah a
  picture of 128 x 128 pixels drawn in 256 tiles of 8 x 8, three a
  frame, in the order of TILE_PERM (CODE:4A1B); at +430Ah the text,
  three characters a frame in tinyfont.fnt (CHAR_PUT: each pixel set by
  the map mask), lines broken at the last space before A0h pixels
  (LINE_MEASURE); the table's five high-score names (TITLE_MAKE) and
  scores (SCORES_MAKE: hex digits of the entry's bytes +6..+9, +4, +5,
  low digit first with dots, drawn right to left by CHAR_PUT_R), a
  character of each a frame.
- GREETINGS_PAGE: GREETINGS_TEXT from CODE, two characters a frame,
  lines of 128h pixels; black with white text.

After Esc a fade to CODE:8A3C, PAGE_MODE_END (the chooser's CRTC pairs
from CHOOSER_CRTC, VIDEO_TOP_CLEAR), SCROLL_INIT, the attract palettes
again and the next backdrop drawn by CUBE_DRAW_MIX (CUBE_DRAW with
MUSIC_MIX after each rotation, no FRAME_WAIT), then MENU_LOOP again.

Runs 2026-09-30, Linux (NOSOUND.SDR as in "The driver's timer"):

- Enter at 112, Right at 114, Enter at 115 (the Info page of Law 'N
  Justice), `-break 1034EB#900` (FRAME_WAIT's 900th call, t=123.189670)
  against the port with DK_FRAMES 6300 (899 frames of the chooser) and
  the keys at pictures 5637, 5756, 5815: CODE only FRAME_SPINS, TAIL and
  video memory equal.
- The four keys of KEY_HISTORY at 110, 110.5, 111, 111.5 (`-key 110
  tab -key 110.5 3A -key 111 lshift -key 111.5 lctrl`; pictures 5518,
  5548, 5578, 5607): the greetings page, the same comparison at the
  900th FRAME_WAIT: the same.
- Then Esc at 125 and Enter at 130 to table 1 (`-break 10B253`,
  t=130.769651): with Esc at picture 6407 and Enter at 6695 CODE differs
  only in FRAME_SPINS and VSYNC_COUNT (3Ah against 30h), TAIL and video
  memory equal, after either page. With Enter at 6705 (VSYNC_COUNT
  equal) the menu's scroll and ATTRACT_TIME are 10 frames apart: the
  original spends about 10 retraces in CUBE_DRAW_MIX's CPU time, which
  counts VSYNC_COUNT but no loop frames; the port draws it at once. The
  same holds for CUBE_DRAW at the chooser's start (CHOOSER at t=107.85,
  CHOOSER_WAIT at 108.02). Not modelled: on another CPU it is another
  time.

The port's pictures at 6290 of both pages looked at: the picture, the
text, the names and the scores (1.000.000.000 down to 50.000.000) of
Law 'N Justice; the greetings text. Not checked: the other three tables'
Info pages, a text longer than the page.

### The table's start

TABLE (CODE:A323; names in src/ILLUSION.hints, "the table's start"), AL
the table 1..4: DS and CS get aliases (INT 93h AH=8; in the run DS 04h,
CS 0Ch with limit 200000h, the far jump through TABLE_JUMP to TABLE_CS),
TABLE_VIDEO_SEL, the two lists BLOCKS and ALLOCS cleared, TBL_KBD_INSTALL
(IRQ 1's old vector 0030:00004152, KBD_IRQ set, IRQ 1 still masked),
TABLE_CHECK (the dword at CODE:90A3 less the byte sum of INTRO_MODE up
to INTRO_SCRIPT: 2A976h, presumably a jump target as in the chooser's
start; not followed), the state pointer [14h] = CB3Eh, TABLE_DIGITS
(the table's digit into eight file names), TABLE_LOAD (state+21E6h's
258h words 0, 2Ah, ...; TABLE_SOUND: the driver started again as
SOUND_START does, with a second copy of the host callbacks at
TBL_CALLBACKS, whose file callbacks keep their cells at 9944h/9948h;
command 11h hands it JINGLE_END_CB; music.mod and music2.mod loaded,
order 1 of slot 0 played), then TABLE_LOAD2 (CODE:B048), the game
(CODE:B928, IRQ 1 unmasked around it) and the end (CODE:A448 on).

The run (2026-09-30, Linux, NOSOUND.SDR as in "The driver's timer",
Enter at 112 and 115, table 1): CODE:A323 at t=115.78, TABLE_LOAD2
(CODE:A3F0, `-break 10B320`) at 121.00, the game (CODE:B928, `-break
10C858`) at 132.55; no pictures counted up to CODE:A3F0 (the frames
stay 5866; TBL_VGA_INIT counts three, see "The table's display"). The port (port/src/table.c)
against the run at CODE:A3F0 (DK_KEYS "5637:1C 5646:9C 5815:1C
5824:9C"): CODE only FRAME_SPINS, TAIL and video memory equal; of the 55
heap blocks (the driver's, its buffers and the two modules' samples and
patterns) all equal but the driver's SAVED_61 (port 61h at command 0):
00h in the run, 30h in the port. dosrun makes bits 4 and 5 of port 61h
from the emulated clock (the refresh toggle, timer 2's output), so the
value hangs on the CPU time, which the port does not model; the driver
only writes it back at its end.

### The dot-matrix display's blocks

TABLE_LOAD2 (CODE:B048; names in src/ILLUSION.hints, from TABLE_LOAD2
on) first takes "Hidelights mask" (33450h bytes, INT 92h AH=0Ah, not
cleared), loads `data\s00n\special\vm_data.mgl` (not on table 3) and
runs DM_LOAD (CODE:2833E): the dot-matrix display's text and animation
areas (1400h bytes each, and a "Temp" copy of each), the five fonts of
`data\misc` read from the top of the heap (policy 1) and each expanded
into a block four times its size (DM_FONT_EXPAND: a byte's bits 7, 5, 3
and 1 as FFh or 0; bits 6, 4, 2 and 0 are passed over, so each font
byte holds four dots and four unused bits, presumably; the fonts
themselves not looked at), then `data\s00n\anims\allanims.mgl`. What
the 33450h bytes and vm_data.mgl are for is not followed.

The port (src/dotmatrix.c) against the run at CODE:B22D (`-break
10C15D`, t=123.79, frames still 5866): equal but for FRAME_SPINS,
SAVED_61, and 231 bytes in the two blocks taken with AH=0Ah and not
cleared ("Hidelights mask", "Temp Text area"). Those bytes already
differed at CODE:A3F0, when the memory was free, and neither program
writes them before CODE:B22D: leftovers of blocks freed earlier whose
contents differ between the port and the run (which ones not followed).
Whether the game reads them before writing them is not checked; if a
later comparison differs there, that is the place to look.

Found on the way: `port/build/game` moves into the data folder on the
port's first start since the doskit update (see "Start here"); the runs
now take `-game game`.

### The module's load

TABLE_MODULE (CODE:B22D; names in src/ILLUSION.hints, "the table's
module") makes the module's name `source\t00n.bpc` with DEC_TEXT (host
vector +18h: the low three nibbles of [CODE:0020] as up to three
decimal digits) and MODULE_LOAD (CODE:B3F5) does what
docs/bpc-module.md describes: the module, its `.rel` from the top of
the heap, each dword the `.rel` names given the module's DS offset, the
header's 2Dh dwords to MODULE_HEADER (CODE:F3E4). A header dword with
bit 31 set is a resource: the file its (relocated) pointer names
("DATALOAD", the selector added to BLOCKS), or with bit 30 too, two
files named one after the other, read from the top and joined into a
block of INT 92h AH=9 ("DATALOAD 2", its linear address added to
ALLOCS). Either way every relocated dword equal to the pointer becomes
the loaded block's DS offset (REL_REPLACE). Then OPTIONS_APPLY,
BALLS_ON_TABLE 1, the module's slot 39 (a RET; the port checks that the
byte is C3h) and HISCORES_GET (the table's 32h bytes of HISCORES).

INT 92h AH=9 allocates by the policy set (0 here), from the bottom: in
the run of table 1 the two joined blocks lay at 2E5920h and 300910h,
where the port's first fit put them (pmax_alloc_linear_here; AH=6
always takes the top).

Runs 2026-09-30 of all four tables to CODE:911F (keys in port/README.md,
"Checked"; Up in the table menu does not wrap to table 4): the port
equal but for FRAME_SPINS, SAVED_61 and the leftovers in "Hidelights
mask" and a "Temp" area (see "The dot-matrix display's blocks").
Table 2 has 3 more bytes that differ, at +2088h of a block of 2090h
bytes that host callback 0 ("Used by MS32") gave the driver in
TABLE_SOUND: 47h 62h .. 04h in the run, 0 in the port, the same at
CODE:A3F0 already, so before TABLE_LOAD2. Whether the driver wrote them
or they are a leftover (the block's asked size would tell) is not
followed; table 1 has no such difference.

### The table's display

TBL_VGA_INIT (CODE:911F; names in src/ILLUSION.hints, "the table's
display") sets the table's mode: unchained 256 colours, VGA 360x350
(MODE_VGA360: mode 13h, the CRTC words at CRTC_WORDS, the 28 MHz clock
and 360x350 timings, 84 bytes a line, the split at 13Dh) or VGA 320
(OPT_RESOLUTION 3); the SVGA modes stop the port. MEASURE_RATE then
times one picture with PIT channel 0: FRAME_RATE 61 (3Dh, the cap) for
VGA 360 and 59 for VGA 320 in the runs, and the port's
1193182 / vga_refresh_hz() ticks give the same. The stage (STAGE.M,
header slot 19) goes into video memory from 1500h and into a block
"Spooky" of 32DC0h bytes, both through STAGE_BITS: each output byte
takes one bit column of eight source rows 30h bytes apart, so the stage
is stored as bit planes of 30h-byte rows and made into the unchained
layout here (presumably; not checked against the file's format). The
palette is TBL_PALETTE (CODE:AA2B) both times; the stage palette's
pointer (slot 20) is read and overwritten at once.

The run (table 1, 2026-09-30) counts three retrace edges in it (frames
5866 to 5869 at CODE:BA9B, t=130.52): the one TBL_VGA_INIT waits for
and two in MEASURE_RATE; the port waits three pictures. ("The table's
start" said the frames stay 5866 up to CODE:B928; that is not so.)
Against the run at CODE:BA9B (`-break 10C9CB`), with OPT_RESOLUTION 0
and 3 (a copy of build/pm/nosound.cfg with the option re-encoded):
CODE only FRAME_SPINS, TAIL and video memory equal, the heap blocks
equal but the known leftovers and 7 bytes of "Spooky" past the 31380h
it writes. The VGA registers and the DAC are not compared (memcmp.py
has neither).

Not modelled: MEASURE_RATE leaves PIT channel 0 in mode 0, the channel
the driver's timer runs on (mode 3, "The driver's timer"). In the run
IRQ 0 then comes once more and stops, presumably, until something
programs the PIT again; the port's NOSOUND counts IRQs per picture from
PIT_DIV regardless. Whether and where the game or the driver restarts
it is not followed; the driver's sample clock will show it.

### The balls' and flippers' start

BALLS_INIT (CODE:B797; names in src/ILLUSION.hints, "TABLE_LOAD2's
end") fills the 13 ball records at state+10AEh (76h bytes each):
header slots 0-5 into each (BALL_BUNDLE1), SLOPE_X/SLOPE_Y to +3Ch/+3Eh,
then each is listed twice (state+1046h, state+107Ah), numbered 1..13 in
+0Ah and put at the plunger (BALL_PLACE: 11Ch, 1FEh, slots 6-11). The
second coordinate's dword +22h is shifted from an EDI whose high word
the first coordinate left (`MOV DI`), so it holds more than 1FEh << 0Ah
(presumably a slip without effect when only the low word is read; not
followed). The flipper records of header slot 22 get their step counts
(+14h) and signs. Before it TOP_COLOURS_SET sets DAC colours FCh..FFh.

Against the runs of all four tables at CODE:28C45 (`-break 129B75`):
CODE only FRAME_SPINS, TAIL equal, the heap blocks as at CODE:BA9B.

Then LIGHTS_LOAD (CODE:28C45) sets two tables of FFh bytes to 1, clears
the first 31380h bytes of "Hidelights mask" (so from here on only its
last bytes are leftovers) and loads `masks\lights.mgl`, `drops.mgl`
and, but on table 2, `masks.mgl`. FLIPDAT_LOAD (CODE:15030) loads
`data\misc\flipdat1.m` and FLIPPER_BLOCKS (CODE:150B0) makes, for each
flipper record, a rectangle of four words per angle it can take (from
FLIP_SHAPES, CODE:14E4E, 4 bytes an angle) and two cleared blocks sized
by the heights' sum, for FLIPPER_RENDER to draw into. Against the runs
of all four tables at CODE:1527F (`-break 1161AF`): CODE only
FRAME_SPINS, TAIL and video memory equal, the heap blocks equal but the
known leftovers.

FLIPPER_RENDER (CODE:1527F; flipper.c) draws each flipper at each angle
it can take: the box's background kept from video memory (read map
plane by plane), the flipper drawn over it from flipdat1.m (four bit
planes of 8 bytes a row, 64 pixels), read back, and only the changed
pixels kept in the "flipper gfx data" block (unchanged ones 0; the
first row of each plane's columns also gets the row where the column
first changed, where it is still 0), the background put back from
"Spooky", and the collision mask for the angle made in the "flipper
mask data" block: the collision map at the record's +1Ch (2Ah bytes a
line) around the box, the flipper's pixels ORed in. The offsets of
each angle's pieces go into the record (+30h.., +B2h..). MULTIBALL_CAP
then lowers the multiball counts to OPT_MULTIBALL's maximum.

Against the runs at CODE:B0E3 and at the game, CODE:B928 (`-break
10C858`), all four tables: CODE only FRAME_SPINS, TAIL and video memory
equal, the heap blocks (the flippers' 12 or so blocks among them) equal
but the known leftovers; with OPT_MULTIBALL 1 (THREE) on table 1 the
counts 6 and 4 became 3 and 3 in both. So TABLE_LOAD2 is whole in the
port.

### The game's start

TABLE_GAME (CODE:B928; names in src/ILLUSION.hints, "the game's start")
begins with DISPLAY_RESET (the display queue's state), LIGHTS_RESET
(each light of header slot 14's groups reset: off, not blinking; a
light with +2 bit 3 is a drop target, whose picture is drawn by
SPRITE4_DRAW into the stage's copy "Spooky" and into video memory
where the hide-lights mask is 0; the others get their LIGHTS_ONE1 byte
0), DM_CLEAR and SCREEN_START (the CRTC's start at SCREEN_LINE). In the
runs tables 3 and 4 draw six drop targets each there, tables 1 and 2
none (counted in the dumps).

Against the runs of all four tables at CODE:30114 (`-break 131044`):
CODE only FRAME_SPINS, TAIL and video memory equal, the heap blocks
equal but the known leftovers.

Then ATTRACT_SCROLL (CODE:30114: the attract mode's scroll a line on,
between 0 and SCROLL_MAX, which is TBL_D892 renamed; the CRT start for
the retrace routine in CRT_NEXT) and FLIPPERS_DRAW (CODE:156C4): each
flipper whose angle changed is put back from "Spooky" and drawn anew
from the pixels FLIPPER_RENDER kept, a column at a time: a column's
first byte says how many rows to pass over, then rows are drawn until a
kept pixel is 0 (at most 55). That byte is FLIP_DIFF's row number, or,
where the column changed in row 0 already, that row's pixel, which is
then taken as a row count too (presumably a slip; its effect not
looked at). The drawing routine addresses with SI and DI (16 bits), the
putting back with ESI and EDI. At the game's start each flipper is
drawn once (about 1750 bytes of video memory a table); the putting back
is not reached there. Against the runs at CODE:1048B (`-break 1113BB`),
all four tables: CODE only FRAME_SPINS, TAIL and video memory equal,
the heap blocks equal but the known leftovers.

FLIPPERS_STEP (CODE:1048B; names in the hints from FLIPPERS_STEP on)
is FLIPPERS_MOVE four times and FLIPPER_SOUNDS. FLIPPERS_MOVE does the
nudges (state+0D42h..0D4Ch), reads the flipper keys into state+2A7Bh
and 2A7Ch, moves each flipper record of header slot 22 (1F7h bytes,
ended by type 0) and counts the tilt (TILT_STEP). A flipper moves up
only while state+2A7Fh is set (the module object's end, see the
comment at CODE:2CFC8) and its key is down; up, its speed is scaled by
32h / FRAME_RATE, down it is added as it is (not scaled: the fall
presumably takes a little less time at 61 pictures a second than at 59;
not looked at in a run). The left flipper (+0Ah not 0) rests at -1, the right one
at 0. FLIPPER_SOUNDS plays FLIP_UP_SFX or FLIP_DOWN_SFX by SFX_PLAY
(CODE:9F2C: the driver's commands 12h and 9, both now in the port's
NOSOUND) when state+2A7Dh or 2A7Eh changed. Against the runs at
LIGHTS_STEP (CODE:2EF7A, `-break 12FEAA`), all four tables: CODE only
FRAME_SPINS, TAIL and video memory equal, the heap blocks equal but the
known leftovers. Only the rest path ran there (no key; the work cells
CODE:0000..0030 equal): the up path, the nudges' keys, the tilt count
and the sounds (commands 9 and 12h) are not checked in a run yet; the
port's KBD_IRQ still stops it.

LIGHTS_STEP (CODE:2EF7A, as the comment in the hints has it) and
FLASH_STEP (CODE:2ED15: LIGHT_FLASH_STEP and GROUP_FLASH_STEP, a light
and a group flashing by a count, the next taken from a stack of 6-byte
entries) are in lights.c, with LIGHT_ON (CODE:28FBA), EVENT_QUEUE and
GROUP_FLASH (CODE:2EF17). At the game's start they draw nothing (the
video memory as at LIGHTS_STEP) and no group is complete, so of them
only the visiting and the lights-off path ran; EVENT_QUEUE, GROUP_FLASH,
the flashing and the lane change are not checked in a run. Against the
runs at CODE:A654 (`-break 10B584`), all four tables: CODE only
FRAME_SPINS, TAIL and video memory equal, the heap blocks equal but the
known leftovers.

TABLE_FADE_IN (CODE:A654) shows the dot-matrix display (DM_SHOW,
CODE:27D0A: 16 rows of A0h dots, the text area's ORed with the
animation area's, the even dots to plane 0 and the odd ones to plane 2,
a row every second line, from TBL_B922), writes TOP_COLOURS into the
stage's palette (header slot 20) at colour FCh and fades in 32 pictures
from TBL_PALETTE toward the stage's palette (TBL_FADE_PAL, the last step
31/32 of the way; the run counts 32 pictures, frames 5869 to 5901).
Then TABLE_GAME sets CODE:D973 (not followed), GAME_PHASE 1 and
QUIT_TABLE 0. Against the runs at CODE:9B92 (`-break 10AAC2`), all four
tables: CODE only FRAME_SPINS, TAIL and video memory equal, the heap
blocks equal but the known leftovers; table 1's screen at t=133.005
(`-shot`) and the port's picture 5898 (DK_SHOTS) equal pixel for pixel,
the DAC of the fade's second-last step with them. The last step's DAC
is not compared (the port stops before its picture).

GAME_SOUND (CODE:9B92; names in src/ILLUSION.hints, "the game's sound
start") hands the driver DRV_TICK (command 0Eh) and DRV_FRAME (0Fh, ECX
VSYNC_START_ARG), silences the module music (MOD_SILENT: command 0Ch,
NOSOUND's CMD_LEVEL, MASTER_VOL 0), takes the first music record
(MODULE_HEADER+8Ch) into MUSIC_FLAGS and MUSIC_TRACK (CD_STOP, CD_PLAY:
the MSCDEX request 84h from the track's start to the next one's), and
starts the driver's player (command 1, CX 3). DRV_FRAME's pel panning in
VGA 320x240 is 2, 4 or 6 by the sign of the word at CODE:D884 (the
hints said 4 or 6; corrected). On the runs' disc of one data track
(dosrun without -cue) the tracks' starts are FFFFFFFFh (only track 1's
is read from the disc, the others keep what the image has), so
CD_PLAY_START is FFFFFFFFh and the length the default 3:34, 16050
frames; TRACK_LEFT 16050 x 61 / 75 = 13054 frames, on all four tables
(tracks 2, 0Eh, 1Ah, 27h). The port's MSCDEX takes the play request and
plays nothing, as dosrun does without a cue sheet; what a disc with
audio tracks gives is not compared. Against the runs at CODE:B976
(`-break 10C8A6`), all four tables: CODE only FRAME_SPINS, TAIL and
video memory equal, the heap blocks equal but the known leftovers (the
driver's in SAVED_61 only), the DMA buffer as at CODE:9B92. No retrace
came between command 1 and CODE:B976, so DRV_TICK and DRV_FRAME did not
run there.

### The attract mode's frame

The loop from CODE:B976 calls GAME_PHASE's routine (PHASES) until
QUIT_TABLE; phase 1 is ATTRACT (CODE:2A4F8; names in src/ILLUSION.hints,
"the attract mode's frame"). Already in the attract mode one ball lies
on the table (state+0D32h 1 at CODE:B976 on all four tables), so the
first frame reaches the balls' drawing (below).

ATTRACT begins with FRAME_STEP (CODE:29889): FRAME_DONE 0, then the four
routines of the table at [FRAME_ROUTINES] (29912h: FRAME_MUSIC, DM_SHOW,
LIGHTS_DRAW, DROPS_UPDATE) as long as FRAME_DONE has not become FFh
(DRV_FRAME at the retrace), then the wait for it. In a run of table 1
(`-log` at CODE:29889, 298B3 and 298C5 from t=133 to 140) 489 frames
began and 487 ran all four routines before the retrace (the others: the
last one, cut off by the run's end, and one more); the port has no
interrupts, so its retrace comes only in the wait and the four always
run. LIGHTS_DRAW and DROPS_UPDATE also stop within their loops at
FRAME_DONE and go on from there the next frame (LIGHTS_DRAW_POS,
DROPS_UPD_POS); in the port they never stop. Where a frame's lights take
longer than a picture in the original, the port draws them in one go:
not seen yet, not compared.

FRAME_MUSIC is MUSIC_UPDATE (CODE:9DC7) and the driver's command 6.
MUSIC_UPDATE counts TRACK_LEFT down (MUSIC_COUNTDOWN) and handles a
jingle's end, a track in MUSIC_NEXT and the module request (see the
hints); only the countdown ran in the first frame. The paths with the
driver's commands 2 and 0Ah stop the port (not in its NOSOUND yet);
CD_VOLUME (IOCTL output 3) is taken by the port's MSCDEX and does
nothing.

LIGHTS_DRAW (CODE:29033) draws each light whose LIGHTS_ONE1 byte changed
(LIGHTS_ONE2 the byte drawn) from LIGHTS_SEL (`masks\lights.mgl`: a
dword count, a dword table of offsets, each picture x, y, width, height
and the bytes, four planes one after the other) by LIGHT_SPRITE into
"Spooky" and into video memory where the hide-lights mask is 0. A light
off takes picture n plus half the count, and `ADD ESI,EDX` moves the
loop's own index there, so the lights between are not looked at in
that pass (a slip, presumably; the run's LIGHTS_ONE2 from index 2 on
stayed as it was in the first frame, which the port did not do until
it followed this; 31 to 42 lights had changed before the first frame,
light 1, off, was drawn, and the pass went on from 1 plus half the
count; what it met there not looked at). DROPS_UPDATE (CODE:28F41) is the same for the 64h
entries of DROP_PIECES with DROPS_SEL's pictures by SPRITE4_DRAW; the
words it puts into SPR_X and SPR_Y go through AX, EAX's high word 0
there (the driver's command 6 gives back EAX 6; SPRITE4_DRAW leaves a
small value). No piece had changed in the first frame: its drawing is
not checked in a run.

Against the runs at CODE:298C5 (`-break 12A7F5`, the first frame's
wait over), all four tables: CODE only FRAME_SPINS, TAIL and video
memory equal, the heap blocks equal but the known leftovers; the
driver's block in SAVED_61 and in its timer's state (SAMPLE_POS,
TIMER_COUNT, VSYNC_PHASE: the port counts a picture's timer IRQs at
once, dosrun stops right after DRV_FRAME).

### The balls' sprites

After FRAME_STEP's wait (CODE:298C5) come BALLS_STEP (CODE:14DE2: each
ball on the list at state+1046h taken off, BALL_ERASE, and its mark in
the hide-lights mask cleared, BALL_UNMARK), DROPS_DRAW (CODE:28EA6: the
drop targets whose state changed drawn and their masks put into or
taken out of the level's collision map, DROP_MASK), BALLS_SHOW
(CODE:14C46: the lowest ball on the stage to state+0D5Ah/0D5Ch, then
each ball drawn, BALL_DRAW, and marked, BALL_MARK) and FLIPPERS_DRAW
(names in src/ILLUSION.hints, "the balls' sprites").

The ball's picture is code: each table has a routine that draws it
(BALL_DRAWERS: CODE:36320, 37F20, 39B20, 3B720, 816 instructions each)
and one that takes it off (BALL_ERASERS: CODE:371A0, 38DA0, 3A9A0,
3C5A0, 512 each), with the colours as immediates (`MOV BYTE PTR
[ESI+d],colour`, where a bit of the byte at EBP, the ball's +58h, is 0;
map mask and read map changed between the planes); the mark in the
hide-lights mask is four more such routines (BALL_MASKS, CODE:2601D,
26253, 2648C, 266C5, by x AND 3: 92 MOVs of AL, AX or EAX to [EDI+d]).
These are pictures of the game, so the port does not copy them:
port/src/codeint.c runs them from the loaded image, as the chooser's
captions are run, knowing only the instruction forms these twelve
routines are made of (about 50, listed from the image with capstone:
MOV, TEST, AND, ADD, ADC, SUB, CMP, XOR, IMUL, MOVZX, ROL/ROR by 1,
SHL/SHR, INC, DEC, PUSH/POP of 16-bit registers, OUT DX,AX, JAE, JNE,
RET); any other stops the port.

DRV_TICK's time: after the start-up (in the run of table 1 the first
retraces after command 1 came at 14, 28 and 28 ms), the retrace comes
1.3 ms after DRV_FRAME, while FLIPPERS_DRAW runs (`-log` at DRV_TICK,
DRV_FRAME, CODE:298C5, 14C46, 156C4 and ATTRACT's calls, t=133.0 to
133.2). The port's timer gives the retrace at the next picture's start
(nosound.c counts a picture's IRQs at once), so FRAME_COUNT was one
behind at CODE:2A544; FRAME_STEP now ends with ns_retrace(), the waiting
retrace's callback at once. Whether the start-up's longer gaps (two
frames without a retrace in the run) give the port's FRAME_COUNT a lead
later is not checked yet (a comparison some frames on).

Against the runs at CODE:2A544 (`-break 12B474`, after ATTRACT_SCROLL
and FLIPPERS_STEP of the first frame), all four tables: CODE only
FRAME_SPINS, TAIL and video memory equal (the ball drawn), the heap
blocks equal but the known leftovers (the ball's mark in the hide-lights
mask equal), the driver's block in SAVED_61, SAMPLE_POS and
TIMER_COUNT. In that frame no ball was taken off (+70h 0 before the
first drawing) and no drop target changed: BALL_ERASE's routines and
DROP_MASK are not checked in a run yet.

### The attract mode's display

After FLIPPERS_STEP ATTRACT looks at state+0E34h (0 in the attract
mode's start: the display branch at CODE:2A690; else texts at
CODE:2A557, not followed yet): DISPLAY_RUN (CODE:2F35C), then
ANIMS_STEP (CODE:27A0E), then by state+0E35h (1 at the start) the
attract mode's display record (state+292Ah) queued with DISPLAY_QUEUE
(CODE:2FEDB) and state+0E35h set to 0; 0 with no record running sets it FFh and goes
to the high-score pages (CODE:2A6FA, not reached yet). Then
ATTRACT_KEYS (CODE:2A7FD): Esc (state+0E47h) to CODE:2A8AB, F1..F8 or
keypad Enter start a game (GAME_PHASE 2, the number added to
state+0D70h). Names in src/ILLUSION.hints, "the attract mode's
display"; the code is port/src/display.c.

DISPLAY_RUN: a waiting count (state+2A40h, or +2A3Eh while +2A3Ch is set
and no animation plays) is counted down; else the running record
(state+2A2Eh) or the next from the ring of 64 at [state+2A2Ah]
(state+2A28h the reading end) runs one opcode a frame: the word at the
record's +6 plus its position +4, looked up in DISPLAY_OPTAB (a dword
an opcode: the routine's offset from DISPLAY_OPCODES and the opcode's
length, which is added to +4); opcode 0 ends the record. With no record,
the background stream (state+2A58h) runs all its opcodes to its end in
one frame. The routines get the record in [0000], the opcode's address
in [0004], the routine's offset and length in [0020], [0024], the
current player and its bit (state+0D72h, 0D74h) in [0038], [003C].
DISPLAY_QUEUE: a record whose first word has bit 0 becomes the
background stream; else its priority (bytes +2, +3) against the
queue's (state+2A3Ah, 2A3Bh): lower, not queued; higher (or +2
negative), the queue emptied and the display reset first; equal in +2
and higher in +3, only added.

ANIMS_STEP with no animation and no background animation writes through
a null pointer: `MOV AX,[EBX+22H]; MOV [EBX+12H],AX; MOV DWORD PTR
[EBX+16H],0` come between `CMP EBX,0` and its `JE`, and both ways lead
to the RET, so CODE:0012 gets the word at CODE:0022 and CODE:0016..0019
are cleared each such frame (presumably a slip; the port does the
same, and CODE compares equal with it).

Against the runs at the first display opcode's routine (CODE:2F9CF,
opcode 1, an animation; `-break 1308FF`, in the second frame), all four
tables: CODE only FRAME_SPINS, TAIL and video memory equal (the ball
taken off and drawn again: BALL_ERASE's routines ran), the heap blocks
equal but the known leftovers, the driver's block in SAVED_61,
SAMPLE_POS and TIMER_COUNT.

Opcode 1 (OP_ANIM) makes its record state+2A50h's animation, opcode 7
(OP_WAIT) a wait of its word times FRAME_RATE frames; opcodes 4, 11h,
15h, 16h and 17h are bare RETs. ANIMS_STEP then plays the animations on
the list from state+2A50h (CODE:27ABD): one frame each a call by
ANIM_FRAME (CODE:27C2A) into DM_ANIM's block, 160 bytes a line, its
frames one after the other in DM_ANIMS_SEL. Against the runs at opcode
2 (CODE:2F7A5, `-break 1306D5`), all four tables: TAIL and video
memory equal, the same heap blocks equal as at CODE:2F9CF, CODE but
FRAME_SPINS and FRAME_COUNT: the port's 226h, the run's 225h.

FRAME_COUNT at a given point of a frame depends on the machine's speed,
not on the game: in a run of table 1 (`-log` at DRV_TICK, DRV_FRAME,
FRAME_STEP, its wait's end CODE:298C5 and DISPLAY_RUN, t=133 to 150),
DRV_TICK came 1.32 ms or more after DRV_FRAME, the median of 1185
1.335 ms, the longest 15.6 ms (which ones were long not looked at), and
DRV_TICK and DRV_FRAME each came 549 times in the 549 frames up to
opcode 2; in the port (a scratch build printing the four) DRV_FRAME 549
times and DRV_TICK 550, the last at the 549th frame's end. Where the
tick lands in the code is how long the work after the wait takes: in frames 1 to 7 (the display's first opcodes and
animation frames) 1.9 to 16.8 ms from the wait's end to DISPLAY_RUN,
so the tick came before DISPLAY_RUN; from frame 8 on 0.71 ms, so
DISPLAY_RUN saw the count before the tick (1178 of 1185 frames) and
the tick came in the next FRAME_STEP's routines. The break at opcode 2
was in such a frame. The port gives the tick at FRAME_STEP's end, the
place of the heavy frames (and of the game's, presumably, with the
ball's physics after the wait; not checked); so FRAME_COUNT is taken
as timing, like FRAME_SPINS, in the comparisons: one apart where a
frame's work after the wait is shorter than 1.33 ms of the runner's.
What reads it: display opcode 1Ah (a text's blinking) and TAKE_HANDLERS'
handler 1Ah (a random award by its low byte); a frame's difference in
either is not a difference of the game's.

Opcode 3 draws a text record (CODE:27783; words x, y, font, alignment,
then the text, 0-ended) into DM_TEXT, A0h bytes a line: alignment 0
from x/2, 2 centred there (the widths summed first with the font's
tables at CODE:2773B, 27753, 2776B), 1 ending at x from the line below,
drawn from the last character back; another alignment draws nothing
(CF). Each font's routine (the dwords at CODE:27723: 27338, 2737D,
273C2, 27407, 2744C twice) sets the map, glyph and width tables, the
font's selector (DM_FONTS), its line length and rows in
CODE:27321..27333 and draws one character (CODE:27289): its width from
the map (CODE:2726D, halved), its glyph from the character less 20h, a
space moving by the word before the glyph table. A character missing
from the width map leaves DL as the last one set it (CF not tested);
the port carries DL the same way and stops where none set it yet. The
attract record (table 1, run to CODE:2A6FA) drew 11 texts; the first,
in the 5-row font, is equal in DM_TEXT on all four tables one frame
later (`-break 130A47#2`, the port stopped there by a scratch build).

### The high-score pages

When the attract record has ended (no record running, state+0E35h 0)
ATTRACT sets state+0E35h FFh and HISCORE_PAGE, HISCORE_WAIT (CODE:2A6E4,
2A6E6) 0; while state+0E35h is negative HISCORE_PAGES (CODE:2A6FA) runs
in its place: with HISCORE_WAIT 0 the display cleared and the entry
HISCORE_PAGE of TABLE_HISCORES (state+0CFCh, 10 bytes: the initials, a
0, the score's high word at +4, its low dword at +6) drawn, then
HISCORE_WAIT 3 x FRAME_RATE; each frame after that counts it down, at 0
the next entry, and after the fifth state+0E35h 1, so the next frame
queues the attract record again. ATTRACT_KEYS runs in every case.

The page: the score by DM_HISCORE_DRAW (CODE:275D7), centred at x 140h
on line 2 in font 1, then HISCORE_TEXT (CODE:2A968: the place '1' +
the entry, a space, the initials) from the left. DM_HISCORE_DRAW and
DM_SCORE_DRAW (CODE:275E4, a player's record, the display's score and
display opcodes 6 and 8) copy the number into DM_BCD (CODE:26ECB, 8
bytes, two 0 bytes first) in different byte orders (DM_BCD_HISCORE,
DM_BCD_SCORE), then CODE:275EF writes it as text: leading zero nibbles
skipped (one digit at least: the loop is a do-while), a comma before
each three digits, built backwards into the 27h bytes before
CODE:27711, the text record's four words (from CODE:002C..0038) in the
8 bytes before the text, DM_TEXT_DRAW, CLC; a nibble above 9 gives CF
and draws nothing. The `MOV EDI,26ED3h` at CODE:275EF is DM_BCD's end,
not DM_FONTS_LOAD, which begins there (a `num` hint now; it was a `ptr`
hint). The default entries of table 1 read 1,000,000,000, 500,000,000,
250,000,000 and so on (the run's dump).

Against runs (all four tables) one frame after the first page
(CODE:2A7D4, `-break 12B704`) and after the fifth, the attract record
queued again (CODE:2A6C1, `-break 12B5F1#2`), the port stopped at the
same places by a scratch build: CODE only FRAME_SPINS and FRAME_COUNT
(the port one ahead, as at the display opcodes), TAIL and video memory
equal, the heap blocks equal but the known leftovers, DM_TEXT with
them. With no key the attract mode now goes round for ever; FRAME_STEP's
wait stops the port when the window closes (as the chooser's FRAME_WAIT
does), so `DK_FRAMES` ends a headless run: 9000 pictures, 3098 table
frames, 1.5 s for 7200 pictures.

### The table's keyboard handler

KBD_IRQ (CODE:A076) keeps KEY_DOWN (FFh while down, +80h after E0h),
KEY_TOGGLE (inverted on each press), LAST_KEY and CODE:CC2F (the last
key pressed) and KEY_RELEASED (0 after a press, FFh after a release);
a press of Space or Alt (39h, 38h) while KEY_RELEASED is 0 is dropped
before the E0h handling, so KEY_E0 stays as it was: after a dropped
Right Alt (E0 38h) the next key would count as an E0 key (from the
code; not tried). The port hands it each byte as frame.c gives them
(src/table.c).

Against runs (table 1) with F1 and with keypad Enter at t=150 (dosrun
`-key 150 3B`, `E01C`), at GAME_PHASE 2's routine CODE:2A976
(`-break 12B8A6`): with the key at picture 7087 of the port only
FRAME_SPINS and FRAME_COUNT differ (4A2h in the run, 4A3h in the port,
one ahead as before), TAIL, video memory and the heap blocks equal but
the known leftovers; at picture 7086 FRAME_COUNT is equal but the
attract scroll a line behind (ATTRACT_LINE, SCROLL_LINE), so a key
belongs at the port's picture whose FRAME_COUNT is one ahead of the
run's there. With these keys the port's picture is FRAME_COUNT + 5900
(7087 and 4A3h) and table frame + 5902 (table frame 1298 at picture
7200).

### Esc in the attract mode

ATTRACT_KEYS with Esc (KEY_DOWN+1) goes to CODE:2A8AB instead of its
F-keys, and returns from ATTRACT from there: Esc's and Y's KEY_DOWN and
LAST_KEY cleared, then whole frames (FRAME_STEP, ATTRACT_SCROLL,
FLIPPERS_STEP, the display cleared and "REALLY QUIT TABLE?" drawn; no
DISPLAY_RUN, so the display stream waits) until Y (KEY_DOWN+15h):
QUIT_TABLE FFh, which ends the main loop (CODE:BA7C, a RET to TABLE at
CODE:A3FF); or any other key pressed (LAST_KEY): the display cleared,
state+0E35h 1, the attract record queued again the next frame.

Runs (table 1, Esc at t=150, the port's picture 7087): Y at 152 against
the port's 7157, at CODE:BA7C (`-break 10C9AC`): only FRAME_SPINS
differs (FRAME_COUNT equal at this break); N at 152, at CODE:2A6C1's
second call: FRAME_SPINS and FRAME_COUNT (one ahead). With Space
instead of N (`-key 152 39`) the run had state+0D48h 258h and 0D44h
FF38h, the port 0 and 0: FLIPPERS_STEP makes four moves a frame (+258h
a move while Space is down, to 3E8h, the key then cleared; -C8h after),
so the run's nudge was one frame's, its Space came after the question
frame's FLIPPERS_STEP (in its drawing) and before the LAST_KEY test;
the port's keys come at the frame's wait, before FLIPPERS_STEP, so its
nudge ran a frame earlier and had run out by the break. A key's place
within a frame is not modelled (as FRAME_COUNT's tick); keys without an
effect in FLIPPERS_STEP compare equal.

### A game's start

GAME_START (CODE:2A976, GAME_PHASE 2, from ATTRACT_KEYS): the balls per
game to state+0D36h, the player 0 and its bit 1, the eight player
records cleared, then BALLS_RESET (CODE:29AB3), LIGHTS_RESET,
COUNTERS_RESET (CODE:29FFA: slot 16's counters and slot 17's lists),
RECORDS_RESET (CODE:29ED6: slot 15's records, lit ones onto the lit list
and their lamps on, then STREAMS_RESET, CODE:29E45, which CODE:29CE3
ends with too), DROPS_UP_ALL with DROP_SET, DISPLAY_RESET, a few cells,
the sound record CODE:1009A, the module's slot 40 (state+2946h; a RET
but on table 2, where it resets the tune chooser, docs/bpc-module.md;
the port stops there), GAME_PHASE 6. The port's routines are in
src/play.c.

The runs had F1 at t=150 on all four tables, and the port's key had
to be put where the run's landed: t=150 is another point of each
table's attract mode. The picture was found from the attract scroll
at the break (ATTRACT_LINE, SCROLL_LINE: a picture of the port moves
it a line): pictures 7087, 7063, 7102, 7143 for tables 1..4, where the
scroll and FRAME_COUNT both came out as the run's (at CODE:2A976 the
port's FRAME_COUNT had been one ahead, see "The table's keyboard
handler"; the break lies at another place in the frame). There only
FRAME_SPINS differs in CODE, the rest as in port/README.md.

### GAME_PHASE 6, what it calls

Read from the disassembly only (2026-09-30), nothing run or translated
yet; the next session's starting point (the user chose it next).
CODE:2B1DC: state+0D3Eh (BALL_SAVE) = word state+0E42h x FRAME_RATE,
state+0D2Fh FFh, MUSIC_REQUEST with header slot 34's record
(state+292Eh); then a loop of whole frames (CODE:2B21E), as the Esc
question's: FRAME_STEP, CODE:2B49A (the game's frame step: it calls
among others COUNTER_TIMERS, EVENT_RUN, MODE_RUN, LIGHTS_STEP,
LIT_LIST_STEP, OBJECT_HITS, DISPLAY_RUN, ANIMS_STEP, CODE:30784 with
DROPS_RAISE_STEP and HOLE_EJECT_STEP), CODE:1023E (the flippers'
four FLIPPERS_MOVE and FLIPPER_SOUNDS, and CODE:10540, 1237C, 12C20,
133BF: presumably the ball's move and collisions, not read),
ZONES_CHECK (CODE:2C1DA), CODE:2F2AF (display, sounds), DM_CLEAR, the
texts "PLAYER n" (CODE:2B38A; with state+0D30h set "PLAYERS n",
CODE:2B39C, the count state+0D70h) and "BALL n" (CODE:2B3AE) by
DEC_TEXT and DM_TEXT_DRAW, DM_SCORE_IDLE, FLASH_STEP, CODE:2B3BE (while
state+0D30h is set, F1..F8 or keypad Enter change the player count
state+0D70h, capped at 8 after CODE:2B480; not read further). The loop
ends on Esc (CODE:2B345: state+0E35h 1, GAME_PHASE 1, a module request
state+2A84h..2A8Bh) or when state+0D3Ch is 0 (then state+2A78h,
2A75h 0, GAME_PHASE 4, play). The "calls" lists above were taken by a
rough scan of the CALLs up to far jumps, so some belong to the routines
next to these; check each before relying on it.

Suggested order: stage 2 of CODE:1023E's callees (hints, runs with
`-log`/`-watch` of the ball record's +12h, +14h, +1Eh, +22h while the
ball waits and after Enter launches it, CODE:2F300), whether any uses
the x87 (the runner does not emulate it); then the port from
CODE:2B1DC, MUSIC_REQUEST (CODE:2F85C) first, stopping by name at each
routine not translated, compared as before at a break in phase 6's
first frame (F1 at the pictures of "A game's start").

### The balls' physics

Stage 2 of CODE:1023E, now BALLS_PHYSICS (2026-09-30; names and a
record layout in src/ILLUSION.hints, "the balls' physics"). Read from
the code: four passes a frame; in each, every ball in play is sampled
(BALL_SAMPLE: a ring of 16 x 16 pixels, the edge of the ball, against a
flipper's mask when the ball is in that flipper's box for its angle,
else against the level's map, a bit a pixel, 2Ah bytes a line), and on
a hit BALL_COLLIDE averages the angles of the hit samples into the
normal (+28h, 800h a turn), looks up the surface's number (+32h) under
the contact point and takes that surface's four words and handler
(flippers 1..4, the level change 0Ah/0Bh, bumpers 10h..15h, slingshots
16h..1Fh; 20h and up an object for OBJECT_HITS through +6Ch); then
BALL_BOUNCE turns the speed into the normal's frame, reverses and
scales the part along the normal, adds a bumper's or slingshot's kick
(with points, the event stream and a sound), applies friction across it
and turns it back. After each pass FLIPPERS_MOVE and BALLS_MOVE twice
(the place from the speed scaled by 50 / FRAME_RATE, the slope map and
SLOPE_X/SLOPE_Y into the speed, the balls against each other). So a
frame has four flipper moves and eight ball moves; the waiting ball in
GAME_PHASE 6 goes through all of it too.

No x87 instruction is in the program's listing (a search of
build/ILLUSION.ASM for the F-mnemonics found none), so the runner's
missing x87 is no obstacle here.

The run (2026-09-30, Linux; NOSOUND as in "The driver's timer": `-put
'\ILLUSION.CFG' build/pm/nosound.cfg -key 112 enter -key 115 enter
-key 150 f1 -key 153 enter`): GAME_PHASE 1 at t=133.01, 2 and 6 at
150.01, 4 at 154.59, 7 at 156.04. With `-watch` on ball record 0
(state+10AEh, linear 10EB1Ch): +32h cleared by CODE:10546 and written
by CODE:11674 (0Dh from t=157.81), +28h written by CODE:11457 from
t=150.30 (while the ball waits), +6Ch set to FFFFh by CODE:1029B once a
frame. The watch shows the low byte only; the other claims in the hints
(the bounce's arithmetic, the bumpers, BALLS_MOVE's pairs, the flipper
kinds at CODE:11AE9) are from reading and not run. The same keys with
the state's own configuration (not NOSOUND) or with `-cue` left table 1
at "Loading" up to t=165 in the screenshots and GAME_PHASE unwritten;
not looked into.

Next for the port: GAME_PHASE 6 from CODE:2B1DC as suggested above,
with BALLS_PHYSICS's routines translated in the order they are called
(BALL_SAMPLE and MAP_SAMPLE, BALL_COLLIDE with the surface handlers,
BALL_BOUNCE, BALLS_MOVE), each compared at a break in phase 6's first
frame; the ball-ball part of BALLS_MOVE and the flipper kinds want a
closer reading first (one ball in phase 6 does not reach the first).

### GAME_PHASE 6 in the port

BALL_WAIT (CODE:2B1DC; names in src/ILLUSION.hints, "GAME_PHASE 6")
and what it calls are in the port (src/play.c, src/phys.c), translated
cell by cell as the code keeps its work cells CODE:0000..003C. On the
way:

- The ring BALL_SAMPLE takes has 17 lines, not 16 (RING_MASKS and the
  stores up to CODE:26009); the hints said 16 until this change.
- Zone type 0 (CODE:2C59C) is what ends the wait: the ball's first entry
  clears state+0D3Ch and 0D3Dh (and the skill shot's state+0D2Fh, after
  queueing header slot 27's or 28's stream), so BALL_WAIT goes to
  GAME_PHASE 4. On table 1 the zone is met at t=155.53, 2.29 s after
  Enter; on table 4 after 0.47 s.
- MUSIC_REQUEST's module request (FFFEh) makes MUSIC_UPDATE call the
  driver's command 2 twice (pause, resume) around command 8. Command 2
  saves the PIC's mask it reads; the run had FCh (IRQ 0 and 1 open).
  The port now keeps the master mask (nosound.c, pic_in21/pic_out21):
  BAh at the start as dosrun answered at command 0, ORed/ANDed where the
  game (ENTRY, the chooser's and the intro's keyboard, TABLE around the
  game) and the driver (TIMER_START's CODE:10FC, TIMER_STOP, PORTS_RESTORE)
  write port 21h. What the mask held before ENTRY is not known.
- The run is slower than the port in this phase: about 170,500
  instructions a frame, two pictures at dosrun's 6,000,000 a second,
  where the attract mode's frames fit in one. So the driver's sample
  clock drifts (TICKS +83 in the run, +70 in the port over 98 frames)
  and a key's picture in the port no longer follows from its time in
  the run by the attract mode's rate: Enter at t=153.24 was found at
  the port's picture 7201 by trying 7199..7203 (only 7201 compared
  equal). The game's state does not depend on it (CODE equal).

Compared (port/README.md, "Checked"): table 1 at the loop's start after
1, 99 and 129 frames (Enter after 113) and at CODE:2B76E; tables 3 and 4
at CODE:2B76E. Every path the port stops at by name was not reached:
the flipper's mask in BALL_SAMPLE (a ball in a flipper's box on its
level), SURFACE_HANDLERS 1..4 (the flippers' kinds), BALL_BOUNCE's
bumper and slingshot kicks, BALLS_MOVE's second ball, SLINGS_STEP's
picture (CODE:28E7B), a hole in HOLE_EJECT_STEP, zone type 4, a zone
object's light record (+0Ah: CODE:2ECCE and 2FD11) and RECORD_DISPATCH,
MUSIC_REQUEST's words below FFFEh, DROPS_QUEUE_STEP's kinds above 1.

Next: GAME_PHASE 4, play (CODE:2B76E), where the flippers meet the ball
(their masks and surfaces, the flipper kinds of CODE:11AE9, read in
"The balls' physics" only roughly), the bumpers and slingshots, the
objects of OBJECT_HITS, the events (EVENT_RUN, MODE_RUN, CODE:2B4B9's
list) and a lost ball (phases 5, 7, 8). Keys for play: the flippers
are Left/Right Shift or Ctrl (state+2A7Bh/2A7Ch, CODE:14757); a run's key
at a picture has to be searched as above while frames are slow.

### GAME_PHASE 4 in the port

PLAY (CODE:2B76E) and a frame's rules (PLAY_EVENTS, CODE:2B4B9) are in
the port (src/play.c, the new src/events.c; names in src/ILLUSION.hints,
"GAME_PHASE 4, play"). What ran was found by the runner's `-cover`: a run
to CODE:2B76E and one to the first flipper-mask sample (below), the
code of the second not in the first grouped by the labels of
build/ILLUSION.ASM (a scratch script); that is what was translated, with
the rest of each small routine as read:

- EVENT_RUN with opcodes 2 (lit for a time), 5 (a record taken: RECORD_TAKE
  with take handler 15h, COUNTER_LEVELS) and 13h (MUSIC_REQUEST); the
  other opcodes and take handlers stop the port by name. MODE_RUN's
  idle path and a mode stream's commands (the same dispatch);
  LIT_LIST_STEP whole; COUNTER_TIMERS, OBJECT_TIMERS whole;
  BCD_COUNTERS_STEP and OBJECT_HITS only as far as nothing is running
  or hit.
- BALLS_LOST, the ball save's serve again (CODE:2B922, GAME_PHASE 7) and
  LOST_RUNOUT (GAME_PHASE 5) translated from reading, not reached yet.
- The port's stop in BALL_SAMPLE was named CODE:12409, which is inside
  an instruction; the flipper-mask path begins at CODE:1241E (after the
  box's flag +1F6h at 1240B and the level check), now so named.

Compared (port/README.md, "Checked"): table 1, F1 and Enter as in
"GAME_PHASE 6 in the port", at CODE:1241E (t=156.050743, 0.52 s into
play, where the port stops): equal as before. The ball had not been lost
then: the lost-ball paths, LIT_LIST_STEP's removal of a record and
LAMP_OFF did not run. In the coverage run to GAME_PHASE 7 (t=160.34,
the ball lost under the ball save, no flipper pressed) the ball went
through the flippers' boxes (the flipper-mask sampling, CODE:12453, and
the flipper surfaces, CODE:1187F on, ran), which is the next step.

Then (same day) the flippers and the lost ball:

- FLIP_SAMPLE (CODE:1241E): the ring against the flipper's mask, the
  same sampling as MAP_SAMPLE with 16 bytes a line; the mask by the
  angle +1Ah (a negative one mirrored to -n-1) through the words at
  +0B0h, placed by FLIP_SHAPES' two bytes for (+6 + +1Ah) mod 78h.
- The flippers' surfaces 1..4 (SURF_FLIPPER, CODE:1187F): the turn +10h
  lessened by half a weight from the table CODE:21981 (by the contact
  point's distance from the flipper's +2, +4); the eight kinds of
  CODE:11AE9 are one routine repeated every DBh bytes with its own
  8-byte table by the normal's octant (FLIP_KIND0 in the hints). With no
  flipper pressed the turn is 0 at rest, so mostly nothing is pushed;
  kinds 0's and 4's pushes ran all the same (the flippers falling back,
  presumably).
- GAME_PHASE 7 (CODE:2BAD5, SAVE_SERVE in the port): frames with "DON'T
  MOVE" until the served ball leaves the lane.
- For tables 3 and 4: event opcodes 4 (a drop target), 17h (a jump
  unless lit) and, read and translated with them but not run, 1, 0Dh,
  0Eh, 19h; a zone's light state (LIGHT_QUEUE, CODE:2ECCE, and SCORE_ADD,
  CODE:2FD11); RECORD_DISPATCH (CODE:3007A) with type 5, SFX_NOTE
  (CODE:9F88: the driver's command 7, a note of a module's sample; in the
  port NOSOUND's CMD_NOTE, CODE:0DD5).

Compared (port/README.md, "Checked"): table 1 at GAME_PHASE 7 (t=160.34)
and tables 1, 3 and 4 at the first GAME_PHASE 5 (CODE:2BBC6; t=171.28,
165.68, 165.21: two balls lost on table 1, the served ball's too): equal
but the known leftovers. Not run in these games: flipper kinds 1, 2, 3,
5, 6, a push of kind 0 with a negative turn, the weight at or above 2Eh
(CODE:11AAC), opcodes 1, 0Dh, 0Eh, 19h.

GAME_PHASE 5 (CODE:2BBC6) is next: BONUS_ADD (CODE:2BD8C) counts the
bonus and calls the table module's slot 32 (MOD_BALL_BONUS in
src/T001.hints, CODE:37AD there) through state+2926h with the host
vector CODE:2CD10 in [CODE:0010]; after the next ball's resets the
module's slot 41 (state+294Ah). That is the first of the modules' own
code the port meets in play (slot 40 is a RET but on table 2): it has to
be translated per table, T001..T004, from their hints. (Done the same
day, see "GAME_PHASE 5 in the port"; BONUS_ADD is at CODE:2BD95, the
hints' name, not 2BD8C.)

### GAME_PHASE 5 in the port

BALL_END (CODE:2BBC6, GAME_PHASE 5) and what it calls are in the port
(src/play.c, src/lights.c; names added to src/ILLUSION.hints 2026-09-30):
BONUS_ADD, FRAMES_WAIT (CODE:2C037, host vector +0Ch), the next ball's
resets LIGHTS_BALL_RESET (CODE:29BE1), RECORDS_BALL_RESET (29CE3),
COUNTERS_BALL_RESET (2A113), DROPS_UP_BALL, BONUS_CLEAR, and the table
modules' code in the new src/modcode.c: MOD_CALL takes a module routine
by its address (a RET returns; one not translated stops the port as
"table n's module at CODE:X"), the host vector's entries are called by
the address in [[CODE:0010]+n] (so table 4's own copy at 9874h would
work too), and slot 32 (MOD_BALL_BONUS) and table 1's slot 41
(MOD_NEXT_BALL) are translated. A 12-digit BCD add shared by the new
code is `bcd12_add` (src/module.c).

What was learned on the way (from the listings; the runs below agree):

- The four MOD_BALL_BONUS are one routine; with the addresses masked
  they differ only in (1) table 2 has no combos (its count is the
  constant 0, so "n COMBOS" never shows), (2) table 3 picks the
  multiplier's text record by the player's word +12h minus 2, the
  others by its half minus 1, (3) which work cells table 4 stores in
  between (the same values in the end). Each combo is worth 1,000,000
  in the total (the number 1 after the four '0's, C3D5Eh on table 1, is
  added once a combo); the run with a bonus of 12,345, multiplier 4 and
  3 combos paid 3,049,380 on tables 1 and 4.
- The player record's bonus is the 12-digit number ending at +10h
  (bytes +8..+0Fh), the score the one ending at +8; BONUS_ADD adds the
  bonus once, or the multiplier (word +12h) times when that is above 1,
  into the number ending at state+2AD0h; the module copies it to
  state+2AC0h..2AC7h (the total, "ending at state+2AC8h") and adds the
  combos there, and BONUS_ADD pays the total into the score.
- Table 1's slot 41 relights, with a multiplier n, n/2 lamps of the
  chain at 942Eh; an odd n never ends its loop (n - 2 each step, tested
  for 0). The port stops by name there instead of hanging. The
  multiplier is cleared by BONUS_CLEAR just before, unless held (the
  player's byte +14h), so the loop runs only for a held multiplier.
- Header slot 25's first light state (state+290Ah) gets byte +5 FFh at
  the next ball while the player still has an extra ball (its second is
  the ball save's lamp).
- Times in the runs: from the lost ball to the next ball's wait 2.66 s
  with no bonus (t=171.28 to 173.94; "NO BONUS" is shown for 96h
  fiftieths, 3 s, by FRAMES_WAIT), 5.15 s with the poked bonus, 3.97 s
  with two players (BONUS_ADD's own 1.5 s wait on top).
- The runs need the argument `C:\ILLUSION.CFG` to compare with the
  port's memory: without it CFG_NAME holds `ILLUSION.CFG` and 14 bytes of
  CODE differ.
- The heap blocks were compared by a scratch script (not in the
  repository) that walks the run's chain from 1473B0h and reads each
  used block at the same linear address in the port's dump (the port
  keeps no block headers).

Compared (port/README.md, "Checked"): at the second ball's wait
(CODE:2B1DC, second hit) on table 1 without a bonus, with two players,
and on tables 1, 3 and 4 with a bonus, multiplier and combos poked at
CODE:2BBC6 (and written by a scratch hook in the port, since removed):
equal but FRAME_SPINS and the known heap leftovers. `-cover` of those
runs: not run were the extra ball (GAME_PHASE 8), its lamp, game over
(CODE:2BD79), in the two-player run the wrap to the next ball (the
one-player runs took it), a record's lamp in
RECORDS_BALL_RESET, table 1's slot 41 with a multiplier left, the
leading-zero skip's end in the combos' digits, and on the two-player
run COUNTERS_BALL_RESET's carry-over add (it ran on tables 1 and 3);
slot 17's lists had entries only on table 4. The tilt's path through
BONUS_ADD and FRAMES_WAIT's key did not run either. Table 2 still stops
in GAME_START; its slot 41 is not translated (it stops by name).

With Enter every 500 pictures, a whole game on table 1 now plays its
three balls and stops at game over: `Stopped before CODE:2AA90`
(GAME_PHASE 3). Next: GAME_PHASE 3 (CODE:2AA90, "GAME OVER", the high
scores, then GAME_PHASE 1) and GAME_PHASE 8 (CODE:2BB3E, the extra
ball).

### GAME_PHASE 3 and 8 in the port

GAME_OVER (CODE:2AA90, GAME_PHASE 3) with HISCORE_ENTER, OVER_WAIT and
OVER_FRAMES, EXTRA_BALL (CODE:2BB3E, GAME_PHASE 8) and ATTRACT's branch
for the players' scores after a game (CODE:2A557) are in the port
(src/play.c; names in src/ILLUSION.hints, 2026-09-30). With Enter every
500 pictures a whole game on table 1 goes through game over back to the
attract mode ("GAME OVER" and "PLAYER 1" with the score in turn, 100
frames each, the round twice, then the display stream again); F1 and
Enter for a second game after it met a slingshot's kick
(`Stopped before BALL_BOUNCE: a slingshot's kick (CODE:1301A)`).

What was learned (from the listing; the runs agree where they reached):

- The high-score entries (TABLE_HISCORES, state+0CFCh) hold the score
  as a word and a dword (+4, +6), compared with the player record's
  word +0 and dword +4 (so the score is a 48-bit number, those two
  parts, which is how DM_SCORE_DRAW reads it too); a score equal to an
  entry goes above it. The defaults of table 1: ICE 1,000,000,000, ANY
  500,000,000, AJL 250,000,000, SN 100,000,000, KHN 50,000,000 (read
  as packed BCD from the port's memory at game over; not checked
  against the high-score file).
- GAME_OVER writes the initials of CODE:2B09D into the entry its search
  ended at also when no entry was passed; that pointer is then one past
  the fifth entry, state+0D2Eh: MULTIBALL_ON, the skill shot's flag
  state+0D2Fh and state+0D30h get the letters. The letters are spaces
  (20h, the image's) until a name has been typed, so after an ordinary
  game over MULTIBALL_ON is 20h; in the two-player run with the first
  player typing "A" the second player's pass wrote 41h, 20h, 20h. D2Fh
  and D30h are set again before they are read (BALL_WAIT, GAME_START);
  MULTIBALL_ON is not reset by GAME_START, so MODE_RUN's first frame
  of the next game sees it (item 4 of "Next").
- HISCORE_ENTER waits 3 s, shows "PLAYER n GOT A HIGHSCORE" 3 s, then
  takes letters from LAST_KEY through KEY_CHARS a frame at a time until
  three are typed (then 3 s more) or Enter; the name loop's frames took
  two pictures each in the run (28.4 ms).
- EXTRA_BALL sets no ball save (BALL_END cleared it): the extra ball of
  the run, lost 1.7 s after its launch, went to GAME_PHASE 5 at once.
- The players' scores' page count (state+0E2Ah) and CODE:2A928 are the
  player count at game over; CODE:2A92A counts the two rounds.

The heap headers: the extra-ball run is equal to the port up to play's
840th frame (dosrun `-break 12C69E#840`) and differs in the next, in
the ball's record. Bisected by the physics routines' calls (BALL_COLLIDE
#5490.. and BALLS_MOVE #10993.. against scratch counters in the port):
BALLS_MOVE #10995 reads the slope map (the ball's +5Ch, C50h bytes at
linear 2FFCB0h here, 42 bytes a line) at line 75, one past its end, as
the lost ball is below the table's last line (CODE:134D4). There the
original has pMAX's 10h-byte header of the next block ("DATALOAD 2"),
whose byte +0Ch (the name's selector, 04h) is taken as slope vector 4;
the port keeps no headers and read a leftover 2Ch. Earlier lost balls
(the GAME_PHASE 5 comparisons) did not show it, presumably because they
had not reached that line or read an equal byte (not looked at). The
headers seen in the runs: 01, FFh used (00 free), the selector word,
the size (rounded to 16) as a dword, the name's offset dword and
selector word, and a last word that looks like whatever was there
before (not written by pMAX, presumably). The port's pmax.c would need
them written at each allocation (the name's address from each caller;
the driver's own allocations have selector 2Ch as the name's) and at a
free, and the free rest of the chain (the run had one free block from
3E8DA0h up to the top blocks). Done the same day, see "pMAX's heap
headers"; next the slingshot's kick.

Compared: see port/README.md, "Checked" (2026-09-30, GAME_PHASE 3 and
8). Not run: a key KEY_CHARS gives FFh for in the name (read: it is
passed over), a table other than 1,
more than two players, a score equal to an entry.

### pMAX's heap headers

The port keeps pMAX's chain of blocks with their headers since
2026-09-30 (port/src/pmax.c; each allocation names its block with
pmax_name, the offset and selector the original's call has in ESI and
DS). What the runs' memory showed, and the port now does:

- A header: 01h, FFh used or 00h free, the selector word, the size
  rounded to 16 (dword), the name's offset (dword) and selector (word),
  and a last word pMAX leaves as it was. The chain starts at 1473B0h
  (the image's end rounded, minus 10h) and ends at FEFFF0h with 00h FFh.
- The name is DS:ESI of the call: the game's DS was 1Ch before TABLE and
  its alias (04h in the runs) in it; the driver's own blocks and those
  host callbacks 0 and 1 make for it have the driver's DS (2Ch) with
  the callback's name offset in CODE ("Used by MS32" at 6247h, 61FEh,
  in the table's copy 99B8h, 996Fh); callback 6's files "temporary
  file" (629Ch, 9A0Dh) with the DS it is given; INT 94h AH=1 loads have
  the ESI of the stub (for the intro's and the chooser's files the byte
  before the file name, 0; for SETSOUND.DAT the file name itself).
- Policy 0 takes the first free block from the bottom that the rounded
  size fits and leaves the rest as a free block (a new header, selector
  and name 0) after it; policy 1 and INT 92h AH=6/AH=7 cut the block from
  the top end of the highest free block, whose size goes down.
- A free (INT 92h AH=5 or AH=2) writes selector 0 into the header; when
  the block before it is free, that one takes it in and the freed
  block's header stays as it was but for the selector (so still FFh);
  else the block is marked 00h; a free block after it is taken in either
  way. So the chooser's captions (name 2F13h) left headers "01 FF 00 00
  size 2F13 1C" in the table's blocks later: those were the "known
  leftovers" of the earlier comparisons (in "Hidelights mask", "Temp
  Text area", "Spooky"), now equal.
- Not modelled: pMAX's own blocks while it loads a file (FA00h bytes
  from the top, AH=7 from pMAX's code 0030:33A1, name 18h:33A7h; three
  such headers were left in the free space at the top). Their data stays
  in the top's free space and under the headers of later top blocks
  (the last word of "table bin file relocation table"'s header was 2
  on table 1 and 3 on tables 3 and 4 in the runs, something else in the
  port): the only differences in the heap left, besides the driver's
  clock.

Compared (port/README.md, "Checked"): the whole heap range 1473B0h to
FF0000h, byte by byte, at the end of the two-player game over and at
the first lost ball on tables 3 and 4: equal but the driver's clock and
the top's free space; the extra-ball game of "GAME_PHASE 3 and 8 in the
port" now equal through its game over. Not compared since the change:
the chooser's and the intro's stages (the chain there is presumed right
because the table's blocks and the stale headers the chooser left lie
where the runs have them). A block in policy 2's DOS memory gets no
header (none looked at in a run).

### The kicks in the port

BUMPER_KICK (CODE:12F26) and SLING_KICK (CODE:1302B) in BALL_BOUNCE and
PIECE_SET (CODE:28E7B, the slingshot's picture through DROP_PIECES) are
in the port (src/phys.c, src/play.c; names in src/ILLUSION.hints,
2026-09-30). The port's earlier stops were named CODE:12F1D and 1301A,
both inside an instruction (the CMP before the first, the JE before
the second); the kicks start at 12F26 and 1302B.

A kick needs the ball up among the bumpers, which no flipperless game
of the earlier runs reached in the original (a second game after game
over, tables 3 and 4 with Enter every 15 s, flipper taps every half
second on table 1: no kick up to t=250..340). The port met one in a
second game, which was then set up alike in the original: game 1 ended
after its first ball (balls left poked to 1), F1 at the 1080th attract
frame after game over (dosrun `-key 195 3B`, the port's picture 9461),
the first ball launched at the wait's 101st pass (`-key 197.845 1C`,
picture 9561), the second at the 60th (`-key 218.67 1C`, picture
10695, found by trying launch passes in the port): served again once,
then slingshot 1, bumpers 3, 1 and 2 at t=228.48 on. The launch pass
decides the ball's path; how long Enter is held does not (5 or 9
passes gave the same). Compared at game 2's phase-7 entries and at the
second ball's end (`-break 12CAF6#3`, t=233.10): CODE but FRAME_SPINS
equal, video memory equal, the heap as in "pMAX's heap headers"; every
instruction of both kicks and of PIECE_SET ran (dosrun `-cover`).
The pictures were found by counting passes (the wait's by `-log
12C14E`, ATTRACT's by `-log 12B428`, the port's frames by a scratch
print) as in "GAME_PHASE 3 and 8 in the port".

### The objects a ball hits in the port

OBJECT_HITS (CODE:2C7FC) is in the port with its three object types
(src/events.c, 2026-10-01): type 0 (OBJECT_TYPE0, CODE:2C8C2), type 1
(DROP_HIT, CODE:2C9D4) and type 2 (OBJECT_TYPE2, CODE:2CB03); names in
src/ILLUSION.hints. Blind games on table 1 with the flipper keys
tapped every 20..53 frames all stopped there (`OBJECT_HITS: an object
of type 0`) or at zone type 4.

What was learned (from the listing; the run below agrees where it ran):

- The ball's +6Ch is the surface number less 20h (BALL_COLLIDE) for
  surfaces 20h and above. OBJECT_TYPE0 debounces only the objects
  below index 20h (surfaces 20h..3Fh): byte +2 set to 6 and counted
  down by OBJECT_TIMERS; the others are handled every frame the ball
  touches them.
- Type 0 is zone_pay with other offsets: the light state at +4, the
  points record at +0Ch (TAKE_PAY from its +12h, SCORE_ADD from +1Ah
  when the player's light bit was set already), the object's record +8
  (RECORD_DISPATCH), two event streams (the record's +1Ah dword and
  the object's +10h).
- Type 2 sets the ball's speed (+0Eh, +10h) to the object's words +6,
  +8 when the slot-15 record at +2 is lit for the player, and takes the
  record (RECORD_TAKE). Not run: which object of which table is of
  type 2 was not looked for.

Input by frames: doskit's runner has `-keyat ADDR[#N] KEY+|KEY-`
since 2026-10-01 (a key down or up at the Nth pass of an address;
doskit's selftest checks it on HELLO.EXE), so a run and the port get
the same keys at the same frame without matching times. On table 1 the
port's FRAME_STEP count is the picture less 5902 (the chooser's Enter
at pictures 5637 and 5815 as before), and a key the port takes in
frame F's retrace wait is read in that frame as in a run with the key
at CODE:298C5's pass F + 1 (linear 12A7F5): F1 at 1185 and Enter at
1299 gave the states of "A game's start" and "GAME_PHASE 6 in the
port" (equal at pass 1250 and on). Two keys at the same pass do not
work so: the run took the second key's IRQ 1 about 6.2 ms later
(KBD_IRQ at t=156.560334 and 156.566489 for Left and Right Shift at
pass 1450), after that frame's flipper reads (CODE:14757 at 156.5629),
so the right flipper (the fourth record) moved a frame later than in
the port (its +10h, +12h, +1Ah one frame behind from pass 1451 on) and
the ball, hit by it, parted at pass 1454. Why the
second interrupt waits is not followed (presumably IRQ 0 in service
under it: KBD_IRQ's non-specific EOI would then end IRQ 0's instead;
not checked). Keys one frame apart compare.

Compared: see port/README.md, "Checked" (2026-10-01). Not run:
DROP_HIT (table 1's drop targets were not hit in that game; the
earlier reading of 2026-09-29 is all there is), OBJECT_TYPE2, type 0's
branch of a light already set, an index of 20h and above, a timer
running.

### The holes and a whole game in the port

Since 2026-10-01 a blind flipper game on table 1 runs in the port from
F1 to game over and back to the attract mode, equal to a run of the
original where compared. Translated for it (names in
src/ILLUSION.hints):

- ZONE_TYPE4 (CODE:2C719, a hole takes the ball), event opcodes 8 and
  18h (OP_HOLE, OP_HOLE2) and HOLE_EJECT_STEP (CODE:30996) whole, with
  HOLE_BALL_OUT (CODE:30CFB); src/play.c, src/events.c.
- Take handlers 6 (TAKE_COUNT_STREAM), 7 (TAKE_SCORE), 0Bh
  (TAKE_RAISE), 10h (the three in turn) and 14h (TAKE_TIMER).
- Display opcodes 5, 6, 8, 9, 0Ch, 0Eh, 10h, 12h and 1Ah
  (src/display.c), with BIN_BCD (CODE:305E3) and DM_SMALL_NUMBER
  (CODE:1015E, 1010A).

What was learned:

- In a hole's ZONE_TYPE4 with its byte +2 set, the light state at +2Ch
  takes the hole's place in [CODE:0004] for the rest: the ball's
  number goes into the light state's byte +1 and the points and the
  event stream are the light state's +2Ch and +14h, not the hole's (as
  read; that path did not run).
- The hole holds the ball (its byte +1 bit 7, so BALLS_PHYSICS passes
  it over) until the count reaches 0, also after HOLE_BALL_OUT put it
  at the exit at count 3Ch: 60 frames at the exit without moving.
- The own hole's sound record is read from [CODE:0004]+10h (the slip
  in "The holes' eject"); in the run it was 0, so neither the sound nor
  the picture's flicker of that frame came, and the port, which keeps
  the scratch cells, does the same.

Input: doskit's runner has `-keysat ADDR FILE` since 2026-10-01 (many
`-keyat` keys from a file, one breakpoint), as -keyat's 64
breakpoints were too few for a game.

Compared (port/README.md, "Checked", 2026-10-01): the game ended at
table frame 5552 (balls lost at 2488, 3782, 5489; two served again
under the ball save). The frames where the run's LIGHTS_DRAW was cut
by the retrace (FRAME_COUNT one ahead, LIGHTS_DRAW_POS 30h) are the
run's timing, as in "The attract mode's display"; the next comparison
was equal again. Not run in that game: DROP_HIT, OBJECT_TYPE2, OP_HOLE2
and the second-hole path, a hole without a picture, the hole with its
light state (ZONE_TYPE4's byte +2), TAKE_COUNT_STREAM's threshold
stream, TAKE_RAISE's cap, display opcodes 5, 8, 0Ch, 0Eh, 10h, 12h,
1Ah (read, translated, not compared).

### The modes and the take handlers in the port

Blind flipper games (keys by table frame as in "The objects a ball
hits in the port") on tables 1, 3 and 4 run in the port since 2026-10-01 without a stop
to the headless build's frame limit (34098 table frames; that each
game had ended by then was looked at in the three compared games
only) for every flipper period tried (29, 33, 37, 41,
45, 49, 53 and 61 frames, Enter every 7th).
Translated for them (src/events.c, src/nosound.c, src/table.c; names
in src/ILLUSION.hints):

- Event opcodes 3 (OP_BLOCK), 9 (OP_MODE_START), 0Ah (a jump), 0Bh
  (OP_BALL_SAVE), 0Ch, 0Fh, 10h, 11h and 1Ch (OP_MODE_WAIT), and
  MODE_RUN's wait: the mode stream's timer state+0D62h counted down
  with its seconds left (big-endian) in state+2A6Eh, which display
  opcode 0Eh shows; on at the wait's position when the timer runs out,
  at once when the record state+0D66h is no longer lit for the player.
- Take handlers 1, 2, 3, 4, 5, 8, 9, 0Ah, 0Ch, 0Eh, 0Fh, 11h, 12h,
  13h, 16h, 17h, 18h, 1Ah and 1Bh (all but 0Dh and 19h, which are in no
  record of the four tables).
- The driver's command 0Ah (a jingle: the music's place kept, the
  jingle's order started) and the call of command 11h's pointer when a
  jingle has ended (JINGLE_END_CB, CODE:9CD4: JINGLE_ENDED FFh).

All tables start their table frames at the same picture: the port's
FRAME_STEP count is the picture less 5902 on each of the four (F1 at
t=150 is frame 1161, 1185, 1200, 1241 on tables 2, 1, 3, 4).

Compared (port/README.md, "Checked", 2026-10-01): table 1 with flips
every 33 frames (game over between frames 5000 and 7000) and table 4
with flips every 33 frames (game over between 11000 and 13000), equal
at every pass looked at but for keys the run takes after the pass
(a key given at pass N is read in the run's frame N, the break stops
before its interrupt). What ran in these games and the 45-frame game
on table 1 (dosrun `-cover`): take handlers 5, 6, 7, 0Bh, 10h, 11h,
14h, 15h, 16h; event opcodes 3, 9, 0Ah, 0Bh, 0Ch, 11h, 18h, 1Ch and
MODE_RUN's three wait paths; display opcodes 8, 0Ch, 0Eh, 10h;
DROP_HIT, OBJECT_TYPE2, the jingle and its end. Not run: take handlers
1, 2, 3, 8, 9, 0Ah, 0Ch, 0Eh, 0Fh, 12h, 13h, 17h, 18h, 1Ah, 1Bh;
event opcodes 0Fh, 10h; display opcodes 5, 12h, 1Ah (translated from
the listing, not compared). Table 2 still stops at GAME_START (its
module's slot 40); the module object of event opcode 14h (MODE_RUN,
CODE:2CFC8) is still a stop, not met on tables 1, 3, 4.

### Table 2 in the port

Table 2 plays in the port since 2026-10-01: its module's slot 40
(MOD_GAME_START, TUNES_RESET: all players' tune choices 0, template 0
over the audio records 0..2 at the module's 1A760h) and slot 41
(MOD_NEXT_BALL: table 1's lamps by the multiplier with the counter at
40AAh and the chain at 994Ah, then TUNE_COPY), in src/modcode.c
(next_ball_lamps now serves tables 1 and 2); event opcodes 6, 7, 12h,
15h, 16h (slot-16 counters), 1Ah (OP_HOLE_SERVE) and 1Bh
(OP_MULTIBALL) in src/events.c.

Compared: a blind game (flips every 33 frames, Enter every 7th; F1 at
frame 1161, chooser Down `-key 114 down`) at passes 2000, 4000, 6000,
8000 (still GAME_PHASE 4 at 8000): CODE but FRAME_SPINS equal except
FRAME_COUNT, the run's 10 ahead at every one of the four (4Ah against
40h at 8000); video memory equal, the heap as before. Where the 10
frames come from: found 2026-10-01, see "Table 2's FRAME_COUNT".
FRAME_COUNT matters: take handler 1Ah (the random award) and display
opcode 1Ah read it.

Not run / open: event opcode 14h's module object (table 2's music
chooser) was not met in that game (done since, see "Table 2's music
chooser in the port"); with
flips every 61 frames the port stops at `BALLS_MOVE: two balls`
(CODE:13597): the multiball needs the ball-ball cases of the physics,
which "The balls' physics" left unread. Opcodes 1Ah, 1Bh, 7, 12h, 15h,
16h not checked by -cover.

### Table 2's FRAME_COUNT

The run's FRAME_COUNT ahead of the port's on table 2 (the blind game of
"Table 2 in the port", flips every 33 frames) is the runner's timing,
not the game's. Found 2026-10-01 with dosrun `-log 12A7F5` (CODE:298C5,
a pass) and `-log 10C8CC` (DRV_TICK) up to pass 8000: the run's
FRAME_COUNT at passes 2000, 4000, 6000, 8000 was 2000, 4003, 6007,
8010, the port's the pass number each time (so not 10 ahead at all
four, as "Table 2 in the port" had it: 0, 3, 7, 10, from these runs). In the run most passes have
one tick between them; a tick pair "2, 0" (a frame cut by the retrace,
caught up in the next) nets nothing; ten passes had two ticks with no
0 after them: 2628, 2636, 2897, 4053, 4673, 4681, 5076, 7051, 7082,
7091. No event, take or display opcode ran in those frames that the
frames around them lacked (the port's log of them, same keys).

Timed with `-log` at the frame's routines (DRV_TICK, PLAY_SCROLL
CODE:301C9, CODE:1023E, FRAME_STEP's four, FLIPPERS_DRAW): in pass
2627 14.8 ms lie between DRV_TICK and PLAY_SCROLL, against 4.3 ms in
pass 2626, and nothing of the game's runs between them; that is about
one picture (14.2 ms) more. Presumably the driver's IRQ handler waiting
for the retrace (VSYNC_TICK, NOSOUND CODE:08E7, interrupts on): its
timer, counted in PIT IRQs, came just after a retrace and waited for
the next; the frame's work after it then ran past the following
retrace, so that frame took two ticks (pass 2628). Not checked by a
log inside the driver. The port's timer (port/src/nosound.c) ends the
wait at the next picture and the game's work takes no time, so it
cannot lose a picture there; the drift between PIT and CRT the original
has on a real machine depends on the machine's speed, as here on
dosrun's 6,000,000 instructions a second. Not done: the same run with
`-ips 12000000` (with it the intro did not reach the chooser by
t=128, so the chooser's keys at t=112..115 missed; not followed).

### The ball-ball physics in the port

BALLS_PAIR (CODE:13597..1445C, the rest of BALLS_MOVE) is in the port
since 2026-10-01 (src/phys.c; the reading in src/ILLUSION.hints at its
name). Read from the listing: the pair is looked at only when both are
on one level and within 11h pixels both ways; the entry of CODE:24C19
for (dx, dy) is 0 for no contact, else a distance-like word (15, 16
and 17 seen), the case 0..7 and a push; the push (a word below 10h)
moves ball B when dy is negative, else ball A. The eight cases are
four bodies over two tests (close in along x or along y), as the hint
says; only ball A's +9 and +1 are tested, not B's.

Found in blind games (the port with a scratch log; flips every PERIOD
frames, Enter every Nth, as in "Table 2 in the port"): on table 2 the
balls touched in 1 of 16 periods (23..83 in steps of 4, Enter every
5th) and in 14 of 72 patterns (periods 21..89 in steps of 4, Enter
every 3rd, 4th, 6th or 9th); tables 1, 3 and 4 had no two balls in
play in the 16 periods with Enter every 5th.

Compared (port/README.md, "Checked"): table 2, flips every 67 (Enter
every 5th; 2 contacts, 1 turn, case 0), 65 (every 9th; 15 contacts,
12 turns, cases 1, 3, 4) and 89 frames (every 9th; 40 contacts, cases
1, 5, 6, pushes at d=15 in frames 6104..6106), at passes right after
the contacts: equal but for FRAME_SPINS, FRAME_COUNT (the run's 1, 4
or 6 ahead; the open question of "Table 2 in the port") and the keys
at a pass after a key. A game with flips every 29 frames and Enter
every 3rd parted at pass 2037, before any multiball: its Enter release
and a Shift press fall at one pass (2031), the runner's two keys at a
pass ("The objects a ball hits in the port"), not the port. Not run:
cases 2 and 7 compared (2 ran in the 29-frame game only, 7 in none
compared); the pushes of B (dy negative) not told apart from A's.

### Table 2's music chooser in the port

Event opcode 14h (CODE:2DBAD), MODE_RUN's branch for a module object
(CODE:2CFC8, MODE_OBJECT in src/events.c) and table 2's TUNE_START and
TUNE_UPDATE (with TUNE_PIC, src/modcode.c) are in the port since
2026-10-01. CODE:2CFC8 clears state+2A7Fh (the flippers off) every
frame the object runs, and calls its +4 only when no event stream runs
and the event queue's and the ring at state+2A2Ah's next entries are
empty; when +4 returns done (ZF clear) the flippers are on again. The
VideoMode data (VM_DATA_SEL) starts with the dword 3 and three offsets
(10h, A10h, 1410h), the three pictures of 160 x 16 TUNE_PIC copies.

None of 90 blind table 2 games in the port (flips every 21..89 frames
in steps of 4, Enter every 3rd, 4th, 5th, 6th or 9th) reached the
chooser, so it was forced as in "Table 2's chooser in a run": at pass
2000 of the blind game (flips every 33 frames, Enter every 7th) record
927Ah's byte +1 set to 1 and zone 34E3h's eight bytes to `00 00 00 00
50 01 3C 02`, by dosrun `-poke 12A7F5#2000` (the module at linear
2ACEF0h in that run) and by a scratch poke in the port at the same
pass. The chooser ran from pass 2191 to 2247 (Enter); both flippers
were held together in it, so the right one took the tune to 1 and the
left back to 0 (template 0 copied). Compared at passes 2150, 2192,
2210, 2230, 2248, 2400, 3000: CODE but FRAME_SPINS, FRAME_COUNT and
LIGHTS_DRAW_POS/DROPS_UPD_POS (the runner's timing, "Table 2's
FRAME_COUNT") and the keys at a pass after a key equal, video memory
equal, the heap equal but for the header word at FEE61Eh as before.
Not run: tunes 1 and 2 chosen (templates 1 and 2), event stream 4CBEh
compared beyond pass 3000.

### The table's end and the chooser again

TABLE after the game (TABLE_END, CODE:A448), DRIVER_RELOAD (CODE:7182)
and the chooser's second start are in the port since 2026-10-01
(src/table.c, sound.c, chooser.c); ENTRY's loop (CODE:033F) goes round:
chooser, table, chooser.

Read from the listing: the table's end fades the stage's palette out in
32 pictures (the driver's command 6 after each), stops the CD and the
driver (commands 2, 2, 3, 5, 0Bh), puts the high scores back
(HISCORES_PUT, CODE:B3A1: the file is written with INT 94h AH=6 only
when they changed; the port stops there, not run), frees every block the
table took and last the selectors made at its start with INT 93h AH=0Dh.
TBL_DRIVER_SEL is one of them: pMAX frees the selector but not the
block. The run's heap at CODE:7182 holds exactly one block, the table's
"Sound Driver" (3500h bytes at 1473C0h); the chooser's driver loaded
again goes after it (14A8D0h), and so does the next table's. Each
round through a table thus loses 3500h bytes of pMAX's heap
(presumably; one round seen). The port's pmax_free_sel keeps such a
block.

The second chooser: CHOOSER_LOADED is 0 then (set 0 at CODE:5040), so
MENUCHAR_INIT loads menuchar.rix (CODE:2840) and CHOOSER_START its other
five files (CODE:4E98), then CD_VOLUME FFh and track 33h; CODE:1534 1
makes FRAME_WAIT restart the module (command 8) every frame until the
driver's TICKS reach 280Ah, then MASTER_VOL 100h. KBD_INSTALL zeroes
the ring's indices (CODE:1903, KEY_READ), which the port had left out
(0 at the first chooser anyway).

Compared (table 1: Esc in the attract mode at t=150, Y at 152, the
port's pictures 7087 and 7157; Enter at 162 and 165, the port's 7406
and 7580): at CODE:7182, at the second chooser's CHOOSER (CODE:4FF9),
at FRAME_WAIT calls 300, 460, 550, 650, at the second TABLE (CODE:A323)
and at passes 1300, 1500, 2000, 3000 of the table's frame (the second
visit to table 1, the passes counted over both): CODE but FRAME_SPINS,
FRAME_COUNT (one ahead) equal, video memory equal, pMAX's heap equal
but the free space, the drivers' SAVED_61 and mixer positions and one
header's last word, as before. The run's seconds and the port's
pictures do not map simply: Enter at 162 needed picture 7406 (found by
trying; 7475 left CAPTION_TIME 7 apart). Not run: a table's end with
changed high scores, the driver's block lost again in a second round
(heap after two tables), Esc from the second chooser (the program's
end).

### The high scores' file

HISCORES_PUT's INT 94h AH=6 (CODE:B3D1, EDX 9Dh = OPTIONS; CODE:5277 and
CODE:07AA make the same call) run 2026-10-01 on Linux: table 1 (keys
`100 space`, `106 enter`, `112 enter`, `118 enter`, `150 esc`, `153 y`,
no `-cue`; the chooser took the Enter at 106 neither here nor with
`-cue`), HISCORES' first byte poked to 58h at TABLE_END (`-poke 10B378
100FD4 58`) so that the table's scores differ and are copied back. The
file in `build/run/state` afterwards: its 20h-byte header and its size
(220h) unchanged, the 200h bytes at +20h OPTIONS (`-dump 100FCD 512` at
CODE:B3D7) stored by the chained rule of "The configuration file's
options", equal byte for byte. The port (pmax_cfg_write) does the same
and writes the file `-cfg` named; with the same poke as a scratch line
and the keys at pictures 5757 (Space), 6076, 6236 (Enter), 7087 (Esc),
7157 (Y) its file was identical to the run's. Not run: a high score
reached by play, the chooser's write at CODE:5277 (what sets
CODE:5E1C is not looked at), the file AH=8 creates when there is none
(the port then writes none).

### The pause in the port

PLAY_KEYS's pause (P, state+0E5Fh; CODE:2B51C) and Esc in it (PAUSE_QUIT,
CODE:2B63B) are in the port since 2026-10-01 (src/play.c; DM_SAVE and
DM_RESTORE in src/dotmatrix.c, SOUND_PAUSE and SOUND_RESUME in
src/table.c; names in src/ILLUSION.hints). Read from the code:

- The pause keeps the display's two buffers (DM_ANIM, DM_TEXT) in their
  temps, silences the module and the CD (SOUND_PAUSE: both levels kept
  in MOD_LEVEL_KEPT and CD_LEVEL_KEPT, SOUND_PAUSED FFh, so
  MUSIC_COUNTDOWN holds), then steps frames of FRAME_STEP and
  PLAY_SCROLL with "GAME PAUSED" and, by bit 8 of PAUSE_FRAMES (every 256
  frames), "PRESS ANY BUTTON TO PLAY" or
  "PRESS ESC TO QUIT". No physics, no flippers, no events. Any key put
  everything back; the levels come back from the kept bytes, the module
  only if its kept level was not 0.
- Esc asks "REALLY QUIT TABLE?" as in the attract mode, but with
  PLAY_SCROLL; another key goes back to the pause, Y sets state+8Dh
  (the table's end) and leaves PLAY_KEYS with the pause still on:
  SOUND_PAUSED stays FFh and the display's buffers are not put back
  (GAME_SOUND clears SOUND_PAUSED at the next table).

Compared (port/README.md, "Checked"), table 1 with the keys by table
frame of "Start here": F1 at 1185, Enter 1299, P 1400 and Space 1750
(passes 1405, 1700, 1760, 1900: in the pause on both sides of bit 8,
and after it), and P 1400, Esc 1450, Space 1500, Esc 1550, Y 1600
(passes 1460, 1505, 1555, and the chooser after it): equal but the
known leftovers. In the chooser after Y the run after 600 FRAME_WAIT
calls is equal to the port after 601 (the run one frame behind there, FRAME_COUNT
one ahead in the table: the runner's timing as in "Table 2's
FRAME_COUNT", presumably; not traced). Not run: a pause with a CD track
playing or the module at a level (NOSOUND, no `-cue`), a pause during a
display stream or an animation.

### The BCD counters in the port

BCD_COUNTERS_STEP (CODE:2EAC9) is in the port since 2026-10-01
(src/events.c, with sbb_das in src/module.c). Event opcode 0Fh starts
such a counter only in mode streams the blind games did not reach, so
the run was forced: table 1 (F1 at table frame 1185, Enter 1299), at
CODE:298C5's pass 1400 the counter at linear 2DC63Ch (the first of
table 1's two, both subtracting: byte +1 bit 0 set) poked to running
with a start value (dosrun `-poke 12A7F5#1400 2DC63C ...`, the same
bytes by a scratch line in the port).

- The step goes byte by byte as the code adds it: +6..+9 from
  +1Eh..+21h, then +2, +3 from +1Ah, +1Bh (bytes +4, +5 untouched); on
  table 1 12340h (BCD) a frame. It ends on a borrow, or when the value's
  dwords +2, +6 are no longer above the end's +12h, +16h (added: have
  reached them); then the end value and byte +0 0.
- DAS as the runner and the 386 do it: the borrow of its first step
  stays in CF.

Compared: value 20000000h in the top byte, passes 1401, 1500, 2000,
3010, 3030, 3100 (it stopped counting after 3010: PLAY_EVENTS not
running then, presumably the ball lost, not looked at); value 01h,
passes 1480 to 1500 (ended in its first frame, by the compare): CODE
but FRAME_SPINS, video memory, the counter's bytes and the heap equal
as before. The end by a borrow alone was not run.

An adding counter, same day: table 3 (chooser Down at 113 and 114, the
port's 5696 and 5740; F1 at table frame 1300, Enter 1400), the counter
at linear 2C14AAh (start 01h, end 10h in the top byte, step 5140h)
poked running at pass 1500 with 09h in its top byte: passes 1501,
1600, 1690, 1700 (ended: value 10h, byte +0 0), 1720 equal as above.
A first try with 0Fh there ended at once in both: DAA makes the
invalid digit 15h, above the end. Table 3 has three adding counters
(2C14AAh, 2C14D4h, 2C14FEh) and six subtracting ones.

### MUSIC_REQUEST's word below FFFEh

The port's stop at CODE:2F906 (a music record's word +2 negative but
not FFFFh or FFFEh) stays: the original then reads past its table of
two at CODE:2F91C and jumps to an address made of code bytes. Looked
at 2026-10-01 in the runs' memory (table 1, 2, 3, 4 at CODE:298C5's
pass 1300 or later; module base from [CODE:A11D]): every record named
by a `music` opcode in tools/event_streams.py's lists and the three
header records at state+292Eh, 293Ah, 293Eh have +2 = 2, FFFEh or
FFFFh. So the game's data does not reach it. Not looked at: records
handed to MUSIC_REQUEST by other ways (the callers that set [CODE:0000]
otherwise were not followed).

### Two more stops out of reach

Looked at 2026-10-01; both port stops stay.

- DROPS_QUEUE_STEP's kind above 1 (CODE:308C0): the queue state+2A60h
  is written by one routine only (CODE:2CA4D on, a drop target hit),
  which stores the kind as the constant 1; the other writers set the
  queue back to CF920h. So no kind above 1 is queued.
- HOLE_EJECT_STEP's level above 1 (CODE:30D6F): every hole named by an
  `eject` or `eject_at` opcode in tools/event_streams.py's lists, in the
  runs' memory of the four tables (as in "MUSIC_REQUEST's word below
  FFFEh"), has the level +0Eh 0 or 1 (table 1: 4D78h 1, 4DB0h 1, 4EAEh
  0, 4F18h 0; table 2: 4AA8h, 4B26h, 4BBEh, 4CDAh 0, 4C50h 1; table 3:
  4494h 1, 4500h 0; table 4: 44A6h, 44DEh, 459Eh, 460Ch, 4678h, 46D2h
  1; offsets in the module). Holes reached otherwise not looked for.

### GAME_PHASE 9 in the port

The tilt's phase (CODE:2B716) is in the port since 2026-10-01
(src/play.c, TILT). Before, a tilt stopped the port with "Stopped
before CODE:2B716" (the user met it on Windows by nudging fast and took
it for a bug of the nudges). Read from the listing: state+2A7Fh and
state+0D3Ah set 0 each frame, then FRAME_STEP, PLAY_STEP,
BALLS_PHYSICS, BALLS_LOST (a lost ball straight to LOST_RUNOUT, no ball
save), PLAY_EVENTS and "TILT" (CODE:2B761) on the display. No
PLAY_KEYS, DISPLAY_RUN, ANIMS_STEP or PLAYERS_KEYS. State+2A7Fh is the
byte FLIPPERS_MOVE asks before it moves a flipper up, so the flippers
stay down (read, as in "The tilt in a run" not seen in the original).

One headless run (table 1, F1 and Enter pressed every 60 pictures
until the game ran, since the table's start moved by up to 200
pictures between runs; Space at 8300, 8306 and 8312, a scratch print
of the phases, since removed): GAME_PHASE 9 at table frame 1226 with
the nudge count F8h (the original's run: F3h at the third nudge),
"TILT" on the display, the ball lost 149 frames later, GAME_PHASE 5
and 6 in the same frame, the tilt flag 0 again. Not compared with a run
of the original byte by byte; the bonus not seen (it was 0).

### The SVGA modes in the port

SVGA_CHECK (CODE:0753) and the mode routines MODE_SVGA640 (CODE:97A4) and
MODE_SVGA800 (CODE:9822) are in the port since 2026-10-01 (src/setup.c,
src/tblvga.c); before, OPT_RESOLUTION 1 or 2 stopped it at SVGA_CHECK.
doskit's runtime vga.c got the VESA modes 100h, 101h and 103h for it
(vga_set_mode_vesa, the runner's register tables, 8 pixels a character
clock, 40 MHz for clock selects 2 and 3; pictures up to 800x600).

- The port answers as the runner's card does (see "The SVGA modes in a
  run"): no S3 BIOS, VESA there, so OPT_SVGA_MODE becomes 1 (640x480) or
  3 (800x600) and the options are written back, also when a mode was
  kept (the kept mode's check fails on the runner: its INT 10h AH=1Bh
  fills nothing, and AH=0 with 1 or 3 sets a text mode without chain-4).
  The port sets no mode in SVGA_CHECK; that the next mode set comes
  before a picture is presumed, not traced.
- The picture: the game narrows the line to 336 pixels and moves the
  horizontal retrace by half the width taken off. vga.c gives the
  mode's whole width with those pixels in the middle and the overscan
  colour on both sides, as a monitor presumably shows it (not seen on
  one). So the window shows the table narrow and tall, square pixels.

Runs 2026-10-01, Linux, headless, a copy of the runner's ILLUSION.CFG
with OPT_RESOLUTION set (`build/run/state/ILLUSION.CFG`, re-chained by
a scratch script): 640x480 and 800x600 pictures with the table above
and the display below the split; the file afterwards had OPT_SVGA_MODE
1 and 3. On 800x600 a game with Enter every 300 pictures played three
balls through GAME_PHASE 4, 5, 6 and 7 (3706 table frames, stopped by
DK_FRAMES). Not compared with the runner's memory; the window builds
not tried; an S3 card's or a BIOS mode (OPT_SVGA_MODE above 13h) not
run.

800x600 played at half speed in the port (reported on Linux, the
original fine): the driver's FRAME_MEASURE (src/nosound.c) took the
dot clock from a fixed table, 25.175 MHz for clock select 2, while
vga.c times mode 103h's pictures at 40 MHz. So the frame the driver
counted was 1.59 pictures long and DRV_FRAME came every second picture
(headless, table 1, 9000 pictures: 1548 table frames against 3098 in
VGA 360 and SVGA 640). FRAME_MEASURE now takes the clock from
vga_refresh_hz (2026-10-01): 3098 table frames at 800x600 too; VGA 360
and SVGA 640 unchanged (VGA 360's memory at the stop 0 bytes differ
from the build before). The window build not played after the change.

### The take handlers forced in a run

The take handlers the blind games did not run, each forced 2026-10-01 by
a poke at CODE:298C5's pass 1300 (table 1) or 1401 (tables 2, 3) in the
runner and, by a scratch line, the port: the skill-shot streams of
header slots 27 and 28 (whose first ball's launch runs one) made to take
a record with the handler wanted, the record lit for player 0 (byte +1
01h). Table 1: the take operand of streams 3779h and 3793h (stream+0Eh);
tables 2 and 3: opcode 5 and the record written at stream+4 (table 2's
streams 37FFh/3811h have a `music` there, table 3's 36D9h/36F3h a
take). Notes for doing it again: a stream's offsets in
tools/event_streams.py's lists count from its first command, which is
at stream+4; its pointers are DS offsets (module base [CODE:A11D] plus
the module offset), not linear addresses.

Records (module offsets): table 1 handler 1 6D4Ch, 8 6D18h, 0Ah 7C64h,
0Eh 788Eh, 12h 7D24h, 13h 8C14h, 18h 7856h (keys of "The BCD counters
in the port"); table 3 0Fh 5A44h, 17h 75C4h, 1Bh 6736h; table 2 2
7402h, 1Ah 716Eh (F1 at table frame 1300, Enter 1400). Each record
reached the handler's dispatch (CODE:2D9DE, dosrun `-log`), handler 1
counted the extra ball (player +10h 1); handler 1Ah took another record
by its random award. Compared at two passes each (1600 and 2000; 1700
and 2100): CODE but FRAME_SPINS, video memory and the heap equal as
before. What each handler then changed was not looked at one by one
beyond that; 17h ran with no mode waiting.

Still not run: take handlers 3 and 9 (in no record), event opcodes 0Fh
and 10h by a stream (BCD_COUNTERS_STEP was run by a poke, see "The BCD
counters in the port").

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
   - MULTIBALL_ON (state+0D2Eh) left 20h after a game over (found
     2026-09-30, see "GAME_PHASE 3 and 8 in the port"): GAME_OVER writes
     the last initials typed (spaces until then) there for each player
     whose score passes no high score; what that does in the next game
     is not followed (MODE_RUN clears it in the first frame and sets
     state+0D51h, read only).
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
     later. Done 2026-10-01: NOSOUND mixes the music into its DMA
     buffer like any of the drivers, so the port plays that buffer as
     the timer passes it (port/README.md, "Checked", sound); not
     listened to yet. Done 2026-10-01: the CD's audio tracks from the
     cue sheet (port/README.md, "Checked", CD audio): the chooser after
     a table and the tables play CD tracks (track 33h there, a table's
     own), which is what the user missed in the port (music gone after
     a table, the table's sound thin). Done 2026-09-29: the driver loaded (the port always loads
     NOSOUND.SDR), INT 93h AH=8's alias (0Ch), CALLBACKS_CS; memory
     equal at CODE:711D (see "The driver loaded, in a run"). Done
     2026-09-29: NOSOUND's command 0 (port/src/nosound.c; see "The
     driver's command 0 in a run"). Done 2026-09-29: NOSOUND's command 4
     (MOD_LOAD, CODE:1C28), host callbacks 6 to 9 and pMAX's allocation
     from the top; memory equal at CODE:75B6, the end of SOUND_START (see
     "The driver's command 4 in a run"). Done 2026-09-29: CHOOSER_LOAD's
     files (port/src/intro.c; see "The chooser's files in a run"). Done
     2026-09-29: INTRO_PALS_MAKE and the script's variables, up to
     CODE:78D9. Done 2026-09-29: INTRO_MODE and INTROPIX_PALS, up to
     INTRO_FRAME at CODE:7925. Done 2026-09-29: the driver's command 6
     (port/src/nsplay.c; see "The driver's player"), INTRO_FRAME and
     INTROPIX_SHOW, up to CODE:795F. Done 2026-09-30: commands 1 and
     0Dh (the timer by pictures; see "The driver's timer") and the
     intro's loop with INTRO_SCRIPT's routines, up to the intro's end
     (CODE:79C3). Done 2026-09-30: the intro's end, the scroller, the
     keys' way out and commands 3 and 8, up to the chooser (CODE:4CFB;
     see "The intro's end"). Done 2026-09-30: the chooser's start, up to
     CHOOSER (CODE:4FF9; see "The chooser's start"). Done 2026-09-30: CHOOSER's first calls
     (CUBE_DRAW, VSYNC_START with NOSOUND's commands 0Eh and 0Fh,
     MUSIC_PLAY), up to CHOOSER_WAIT (CODE:38C6; see "The chooser's
     timer"). Done 2026-09-30: CHOOSER_WAIT's loop, up to CODE:505C (see
     "CHOOSER_WAIT's loop"). Done 2026-09-30: the table menu and the
     chooser's end, ENTRY's end after Esc, up to the table (CODE:A323;
     see "The chooser's end"). Done 2026-09-30: INFO_PAGE and
     GREETINGS_PAGE (see "The Info and greetings pages"): the chooser
     is whole in the port. Next: the table (CODE:A323, then CODE:7182
     and the chooser again), which wants stage 1 of `SOURCE\T001.BPC`
     first (item 5 below). Done 2026-09-30: TABLE up to TABLE_LOAD2
     (CODE:B048; see "The table's start"). Done 2026-09-30: TABLE_LOAD2
     up to the module, CODE:B22D (see "The dot-matrix display's
     blocks"). Done 2026-09-30: the module's load, up to CODE:911F on
     all four tables (see "The module's load"). Done 2026-09-30:
     TBL_VGA_INIT, up to CODE:BA9B (see "The table's display"). Done
     2026-09-30: TOP_COLOURS_SET and BALLS_INIT, up to CODE:28C45 (see
     "The balls' and flippers' start"). Done 2026-09-30: LIGHTS_LOAD
     and FLIPDAT_LOAD up to FLIPPER_RENDER (CODE:1527F; same section).
     Done 2026-09-30: FLIPPER_RENDER and MULTIBALL_CAP: TABLE_LOAD2 is
     whole, the port stops at the game (CODE:B928; same section). Next:
     the game, CODE:B928: its start (CODE:2A30E .. CODE:9B92, the
     driver's command at CODE:9B92, GAME_PHASE 1), then the main loop
     from CODE:B976 with KBD_IRQ (CODE:A076) for the keys. Done
     2026-09-30: the start up to CODE:30114 (see "The game's start").
     Done 2026-09-30: ATTRACT_SCROLL and FLIPPERS_DRAW, up to
     CODE:1048B (same section). Done 2026-09-30: FLIPPERS_STEP, up to
     LIGHTS_STEP (same section; its up path unchecked). Done
     2026-09-30: LIGHTS_STEP, FLASH_STEP and TABLE_FADE_IN, up to
     CODE:9B92 (same section). Done 2026-09-30: GAME_SOUND (CODE:9B92:
     the driver's commands 0Eh, 0Fh, 0Ch and 1, MUSIC_TRACK), up to
     CODE:B976 (same section). Done 2026-09-30: the loop from CODE:B976,
     ATTRACT's start and FRAME_STEP's four routines, up to CODE:298C5
     (see "The attract mode's frame"). Done 2026-09-30: FRAME_STEP's rest
     (the balls by their own code, run from the image; DROPS_DRAW),
     ATTRACT_SCROLL and FLIPPERS_STEP, up to CODE:2A544 (see "The balls'
     sprites"). Done 2026-09-30: ATTRACT's display branch (DISPLAY_RUN
     with its streams, DISPLAY_QUEUE, ANIMS_STEP's empty path) and
     ATTRACT_KEYS, up to the second frame's first display opcode
     (CODE:2F9CF; see "The attract mode's display"). Next: the display
     opcodes as the attract stream reaches them (opcode 1 first, the
     animations of ANIMS_STEP with it), the high-score pages
     (CODE:2A6FA), KBD_IRQ (CODE:A076); a comparison some hundred frames
     on (FRAME_COUNT against the run's start-up gaps). Done
     2026-09-30: display opcodes 1 and 7, the RET opcodes and
     ANIMS_STEP's animation path, up to opcode 2 (CODE:2F7A5; see "The
     attract mode's display"). Done 2026-09-30: FRAME_COUNT's one ahead
     there is the runner's timing (same section). Done 2026-09-30:
     display opcodes 2, 0Ah, 0Dh, 0Fh, 13h, 14h, 18h and 19h (only 2
     run), up to opcode 3 (CODE:2FB17). Done 2026-09-30: opcode 3's
     text drawing (CODE:27783, DM_TEXT_DRAW in dotmatrix.c), to the
     attract record's end (same section). Done 2026-09-30: the
     high-score pages, CODE:2A6FA (see "The high-score pages"): with no
     key the attract mode goes round in the port. Done 2026-09-30:
     KBD_IRQ (CODE:A076; see "The table's keyboard handler"). Done
     2026-09-30: Esc in the attract mode (CODE:2A8AB; see "Esc in the
     attract mode"). Done 2026-09-30: a game's start, GAME_PHASE 2
     (CODE:2A976; see "A game's start"). Next, chosen by the user
     2026-09-30: GAME_PHASE 6 (CODE:2B1DC, the ball waiting for its
     launch) with the ball's physics and collisions it needs, stage 2
     first (see "GAME_PHASE 6, what it calls"; stage 2 of the physics
     done 2026-09-30, see "The balls' physics"; done 2026-09-30 in the
     port up to GAME_PHASE 4, CODE:2B76E, see "GAME_PHASE 6 in the
     port"; GAME_PHASE 4 begun 2026-09-30, see "GAME_PHASE 4 in the port":
     up to the first GAME_PHASE 5 on tables 1, 3 and 4, with the
     flippers' masks and surfaces and GAME_PHASE 7; GAME_PHASE 5 done
     2026-09-30, see "GAME_PHASE 5 in the port"; GAME_PHASE 3, game
     over, and 8, the extra ball, with the players' scores of ATTRACT
     (CODE:2A557) done 2026-09-30, see "GAME_PHASE 3 and 8 in the
     port"; the headers of pMAX's heap done 2026-09-30, see "pMAX's heap
     headers"; the bumpers' and slingshots' kicks done 2026-09-30, see
     "The kicks in the port"; the objects a ball hits done 2026-10-01,
     see "The objects a ball hits in the port"; the holes and what a
     whole blind game on table 1 needed done 2026-10-01, see "The holes
     and a whole game in the port"; next: games on tables 2, 3 and 4,
     and table 1 with other keys, for the paths not run yet; tables 1, 3
     and 4 done 2026-10-01 as far as blind games go, see "The modes and
     the take handlers in the port"; table 2's slots 40 and 41 done 2026-10-01, see "Table 2 in the
     port"; the ball-ball physics done 2026-10-01, see "The ball-ball
     physics in the port"; table 2's FRAME_COUNT behind (1 to 10
     frames) found 2026-10-01 to be the runner's timing, see "Table
     2's FRAME_COUNT"; event opcode 14h's module object (table
     2's music chooser) done 2026-10-01, see "Table 2's music chooser in
     the port"; the table's end after Y and the chooser again done
     2026-10-01, see "The table's end and the chooser again"; the
     high scores' file done 2026-10-01, see "The high scores' file";
     next: the paths blind games did not run; the pause done 2026-10-01,
     see "The pause in the port"; the BCD counters done 2026-10-01, see
     "The BCD counters in the port"; MUSIC_REQUEST's stop found out of
     the data's reach, see "MUSIC_REQUEST's word below FFFEh"; the drop kind and the
     hole level too, see "Two more stops out of reach"; the take
     handlers forced, see "The take handlers forced in a run");
     later table 2's slot 40, and the
     other phases as a game reaches them; the table's end after Y (done
     2026-10-01, see "The table's end and the chooser again").
   - pMAX's heap, needed for that (walked in -mem dumps with 10h-byte
     headers `01, used FFh/00, selector, size rounded to 16, name offset,
     name selector, policy`): a chain from 1473B0h to FEFFF0h, first fit
     from the bottom for policy 0; MOD.INT (host callback 6, INT 92h AH=8
     BL=1) cut from the top (data at FA8060h, header policy word 2); host
     callback 1's block (BL=2, selector 34h) not in the chain but at linear
     13120h, DOS memory (the trace after callback 3 at driver CODE:1258).
     Selectors: the lowest free of 04h, 0Ch, ... in steps of 8, with 14h,
     1Ch, 24h taken from the start; 48h (video) apart. port/src/pmax.c
     does policies 0, 1 and 2 (only the first block of each seen for 1
     and 2). The headers' last word is not a policy: pMAX leaves it as
     it was (the 2 under MOD.INT's was data left there, presumably); the
     port writes the headers since 2026-09-30, see "pMAX's heap
     headers".
   - on from HISCORE_INIT: CODE:757D,
     the chooser CODE:4CFB and the table CODE:A323 (ENTRY's loop), down to
     the main loop (GAME_PHASE's dispatch at CODE:BAD6); then the parts
     already read, each compared with memcmp.py;
   - stage 1 for `SOURCE\T001.BPC` before table 1's code is needed:
     begun 2026-09-30 (see "Stage 1: T001.BPC"; the header's code slots
     reached); done 2026-09-30: the opcode-14h object's methods, and
     with them every byte of code in the module (the event streams'
     handlers are the main program's); done 2026-09-30: the same for
     tables 2, 3 and 4. Open: the data as `words`/names where the port
     needs them (the event streams, slot-15 records, counters;
     tools/event_streams.py lists them);
   - the ball's physics and collisions (CODE:1352D, the map DROP_MASK
     writes, header slot 18): read 2026-09-30 but for the ball-ball
     cases and the flipper kinds (see "The balls' physics"; no x87 in
     the program). Not read yet: the drawing and scrolling in play.

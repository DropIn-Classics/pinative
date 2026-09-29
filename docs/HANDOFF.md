# Handoff

State of 2026-09-29: the repository was made from doskit's template on
top of an earlier analysis (docs/*.md other than this file). The game's
files are listed. The main program, `ILLUSION.386`, is in stage 1:
`src/ILLUSION.hints` rebuilds it byte for byte (doskit reads pMAX images
since 2026-09-29), with almost no hints yet (see "Stage 1" below).

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

doskit's stage 1 (disasm.py, tasm.py, build.py) and its runner are
16-bit real mode only (METHOD.md: "Not in the runner: protected mode
(DOS extenders)"). The game's code is 32-bit:

- `ILLUSION.386` and the `.SDR` drivers are flat 32-bit images run by
  pMAX's protected mode;
- the `.BPC` table modules are 32-bit code linked at load time;
- only the pMAX loader in `ILLUSION.EXE` and `INSTALL.EXE` are real mode,
  and both are packed or self-decoding.

Decided 2026-09-29: doskit gets 32-bit support (it is wanted for later
games too), in the kit with tests (rule 7).

## Stage 1: ILLUSION.386

`python3 doskit/tools/build.py src/ILLUSION.hints`: IDENTICAL, 3761
instructions (11,405 of 0x46480 bytes reached as code), 509 labels,
10 lines as DB, 15 s on the Mac used (2026-09-29). The hints so far:
the two descriptors as segments (`CODE` the whole image, code and data;
`TAIL` the empty one at its end), the entry point's name, and the 10
`raw` lines:

- nine instructions with a 16-bit address and no register (67h, e.g.
  `mov word ptr es:[0x27], 0` at CODE:0667): a USE32 source line cannot
  ask for that address size;
- CODE:31649 `64 66 AD`: FS before 66h, the one such order.

Seen on the way (in doskit's commits): the image's segment-register
stores to memory carry 66h (`66 8C ...`, four of them), which the source
now writes as `MOV WORD PTR [..],DS`; a pointer variable at CODE:8128
is read and then addresses memory (`MOV ESI,[8128h]`), and the kit had
taken the constant stored into it (CODE:7ABC) for code: about 2000
"instructions" of data, now a data label. The earlier scanner reached
12,018 bytes; not compared with the 11,405 here.

Not done: everything the method's stage 1 asks beyond the byte identity
(gaps.py: 41 gaps; the `ptr`/`dptr` of the many 32-bit immediates that
are offsets, which the analysis does not find by itself; names).

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
   - done 2026-09-29: the pMAX image as a program for disasm.py and
     build.py (`pmax` hint, descriptors as segments, selector
     relocations as SEG fixups, write_pmax), tests/flat in doskit's
     selftest, gaps.py and ptrscan.py for 32-bit programs. What
     write_pmax cannot know it writes as ILLUSION.386 has it: the
     header's first word 0, format 1, allocation = image size, the
     entry as an image offset (descriptor 0 has base 0, so either way);
   - then the runner: protected mode or pMAX's services (INT 90h..94h)
     emulated, still to decide.
2. Stage 1 for the main program, on from the above: gaps.py's 41 gaps
   (code reached only through pointers: e.g. the two tables of 11 far
   pointers at CODE:618D and CODE:98FE, written as `DF` lines, whose
   targets are not reached as code yet; presumably the sound drivers'
   eleven host callbacks of docs/audio-driver.md, not checked), offsets
   among the 32-bit immediates (ptrscan.py), names
   (doskit/docs/METHOD.md). A heuristic for offsets in flat 32-bit code
   would belong in doskit.

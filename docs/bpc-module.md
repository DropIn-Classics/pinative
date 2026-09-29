# BPC table-module format

This document describes the table-module loader observed in the main image at
`0xB3F5` through `0xB796`. All names are provisional and derived exclusively
from the installed GOG binaries, their relocation data, and host-side behavior.

Each table supplies a `SOURCE\\T00x.BPC` module and a matching
`SOURCE\\T00x.REL` relocation list. The four decoded modules are different
sizes and have different relocation counts:

| Table | BPC bytes | REL bytes | Relocations | Unaligned relocations |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 73,728 | 6,828 | 1,707 | 934 |
| 2 | 109,056 | 6,604 | 1,651 | 864 |
| 3 | 126,976 | 6,024 | 1,506 | 819 |
| 4 | 104,448 | 6,988 | 1,747 | 1,019 |

`bpc_inspect.py` validates these invariants and prints the header:

```sh
python3 tools/bpc_inspect.py build/files/SOURCE/T001.BPC build/files/SOURCE/T001.REL
```

## Relocation algorithm

The `.REL` file is a headerless sequence of little-endian 32-bit offsets into
the BPC image. The list is not required to be sorted or aligned. It contains no
duplicates in the four observed modules.

After loading the BPC, the loader obtains its linear base relative to the main
flat segment. For every relocation offset `r`, the operation at `0xB743` is:

```c
*(uint32_t *)(module_base + r) += module_base;
```

Thus each relocation identifies the location of a pointer word, not the target
itself. Relative `call` and `jump` displacements do not appear in the list.

## 45-slot header

The first 180 bytes are a fixed table of 45 relocatable pointer slots. The
loader copies the relocated table into main-image state at `0xF3E4`. Null slots
are not present in the relocation list.

Most slots are module-relative data or function pointers. Ten slots have flags
in the top two bits before relocation:

| Flag | Loader action |
| ---: | --- |
| `0x80000000` | Clear bit 31, interpret the resulting module pointer as a resource name, load that resource, and replace every relocated occurrence of the name pointer with the loaded data pointer. |
| `0xC0000000` | Clear bits 31 and 30, read two consecutive resource names, load both resources, concatenate their decoded bytes into one allocation, and replace every occurrence with that allocation pointer. |

The flagged slots have the same roles in all four modules:

| Slot | Kind | Table-relative resources |
| ---: | --- | --- |
| 0 | joined | `LINK1` + `PIXELS1` |
| 1 | resource | `MASK1` |
| 2 | resource | `HIDE1.M` |
| 3 | resource | `ANGLE1` |
| 6 | joined | `LINK2` + `PIXELS2` |
| 7 | resource | `MASK2` |
| 8 | resource | `HIDE2.M` |
| 9 | resource | `ANGLE2` |
| 19 | resource | `STAGE.M` |
| 20 | resource | `STAGE.C` |

The replacement routine at `0xB768` walks the relocation list and changes all
pointer words equal to the resource-name pointer *after* the flag bits have
been cleared. Asset pointers are therefore resolved throughout the module,
not just in its copied header. The `Other relocated aliases` column emitted by
`bpc_inspect.py` uses that unflagged value for resource slots. In the four
modules, `MASK1` has 3, 2, 2, and 3 such aliases and `MASK2` has 0, 1, 1, and
0; the other eight flagged slots have none.

## Module entry and lifetime

Header slot 39, at module offset `0x9C`, is the post-load hook. After resource
resolution, the main loader reads the module base from `0xA11D`, adds `0x9C`,
and calls the pointer at `0xB287` through `0xB291`. It is a one-byte `ret` stub
in all four modules, so calling it has no effect in this release.

Slots 40 and 41 are adjacent callable exports reached through the copied
header. The main-image transition paths at `0x2AA6D` and `0x2BD61` load and
call slots 40 and 41 respectively. These accesses use a runtime-state pointer,
which is why a scan limited to module-base and literal-address references
misses them. Each target occurs in exactly one BPC relocation, its own header
word, and no direct relative BPC call targets it.

Slot 40 is a no-op in tables 1, 3, and 4, while table 2 calls a table-local
routine. Slot 41 contains a substantial routine in tables 1 and 2 and is a
no-op in tables 3 and 4. Slot 42 is null in every module. `bpc_inspect.py`
reports the number of other relocated aliases for every header target; slots
39 through 41 all report zero for every module.

The main image retains the BPC selector, its relative base, all loaded-resource
selectors, and dynamically combined allocations. Cleanup releases these in
reverse lists maintained around `0xA121` and `0xA1ED`.

## Copied-header consumers

The loader writes the relocated header sequentially at `0xF3E4` using the
single base literal at `0xB502`. Runtime state begins at `0xCB3E`, so code that
loads the state pointer from shared cell `[0x14]` addresses the copied header
at displacement `+0x28A6`. This is the normal access form and cannot be found
by searching only for literal header addresses.

`bpc_header_xrefs.py` (an earlier scanner, removed) anchored disassembly at those state-pointer loads and also
checks absolute memory operands. It finds copied-header reads for 37 of the 45
slots: 0 through 20, except 21; 22 through 28; 32 through 38; and 40 through
41. Slots 19, 20, 22, and 35 use absolute addresses; the others use the state
pointer (slot 22 uses both forms). The eight copied slots with no main-image
read are 21, 29, 30, 31, 39, 42, 43, and 44. Of those, slot 39 is called and
slots 43/44 are configured directly through the module base, slot 42 is null,
and slots 21 and 29 through 31 remain module-side data roots.

This was found with an earlier scanner (`bpc_header_xrefs.py`) that is no longer in the
repository (it is in build/earlier/ on the machine it was removed on);
to be checked again in the hints once the 32-bit stage 1 exists.

## Audio-control records

Slot 34 is the base of a contiguous array of 12-byte records. Every observed
record consists of six little-endian 16-bit fields and begins with the value
4. Slots 35 through 38 point to individual records in that same array. The
array sizes and exported indices are:

| Table | Records | Slot 35 | Slot 36 | Slot 37 | Slot 38 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 44 | 2 | 14 | 10 | 13 |
| 2 | 47 | 11 | 35 | 18 | 38 |
| 3 | 41 | 2 | 22 | 11 | 40 |
| 4 | 41 | 3 | 6 | 4 | 7 |

These are audio-control records rather than generic table state. Module event
records repeatedly refer to them, and the end-of-ball bonus export in slot 32
passes one to the host callback at offset `+4`. The main image consumes slot
35 directly during music startup: its word at `+8` is passed into the routine
at `0x9D83`, while its byte at `+0x0A` is stored at `0xCB97`.

The fields, from `MUSIC_REQUEST` (`0x2F85C`) and the music update at
`0x9DC7` (checked 2026-09-29 in the hints; the plays in runs: docs/HANDOFF.md):

| Offset | Meaning |
| ---: | --- |
| `+0` | 4, the record type (`0x3007A` dispatches by it) |
| `+2` | the sound driver's request: 0 none; positive (1, 2, 3, 6, 7, 8 in the four modules' records; the main program reads only the sign, at `0x9E83`) driver command `0x0A`, a temporary module playback: a jingle in place of the module's music, which afterwards goes on from where it stopped (seen in runs 2026-09-29: table 1's record 34 requested at the skill-shot display, and its words alone, 1.78 s; docs/HANDOFF.md, "The jingle alone"); negative (`FFFE`, `FFFF`, handled alike) command 8, the module music from the order index; a negative one first keeps the flags and the asked-for track in state `+0x5A`, `+0x2A82` (read by no one found) |
| `+4` | the order index in the music module (driver `BL`) |
| `+6` | the module slot (driver `CL`): 0 `data\s00n\music.mod`, 1 `music2.mod` (both loaded at the table's start; table 4, which has no slot-30 string, uses 0 only) |
| `+8` | a CD track, 0 none; with a track, `+4`/`+6` are the module music to return to when the track's time is up (order index 0: the track again). The table's first track (slot 35, `0x9BCA`) is played with the return cleared, so it repeats, as seen in the runs |
| `+0A` | flags to `0xCB97`: bit 0 the track's length is counted (`MUSIC_TRACK`), which the return above needs; bit 2 the CD volume 0 before a jingle; bit 1 is added while `0x2BAD3` is set |

So a mode's closing record with track 0 (table 1's record 3, table 2's
record 2, ...) does not stop the music: it switches from the mode's CD
track back to the table's module music at its order index.

The array is mutable runtime state in table 2. Routines at `0x9A55`, `0x9ADA`,
and the path at `0x9CD9` copy 18 words (three complete records) from one
of three templates over records 0 through 2; see "Table 2's music chooser"
below. A compatibility implementation must not treat the slot-34 array as
immutable asset data. `bpc_inspect.py` validates the record run and exported
indices and prints the five exported records.

## Table 2's music chooser

Found 2026-09-29 by disassembling table 2's module (a throwaway capstone
script) and the main program's hints. Seen in a run the same day
(docs/HANDOFF.md, "Table 2's chooser in a run", the chooser forced with
two `-poke`): the texts, the three names with Right Shift, track 19
while it runs and track 14 after "ROLL ME ON" and Enter.

- Table 2 offers three tunes. Its templates are the records 3..5, 6..8
  and 9..11 (pointers at module `0x9D9E`), their names text records at
  `0x9DB6`, `0x9DCC`, `0x9DE6` (pointers at `0x9DAA`): "BY THE BEACH",
  "MOONLIGHT PARKING", "ROLL ME ON". The three differ in the order index
  (`+4`) of records 0..2: 0/1/1, `0x10`/`0x11`/`0x11`, `0x1D`/`0x1E`/1,
  and the third's record 2 carries track 14 as well.
- The word array at `0x9D88` (8 words, one per player by state `+0xD72`,
  the current player) holds each player's choice, 0..2; bytes `+0x10`,
  `+0x11` and words `+0x12`, `+0x14` after it are the chooser's timers.
- Slot 40 (`0xB5`, called at a game's start, `0x2AA6D`) calls `0x9ADA`:
  all eight choices 0, template 0 copied. Slot 41 (`0xBB`, called at the
  next player's ball, `0x2BD61`) relights lamps of the list at `0x994A`
  by the player's word `+0x12` (the bonus multiplier, presumably) and
  calls `0x9A55`: the current player's template copied.
- The chooser is the opcode-`0x14` object at `0x9A08` (start `0x9A10`,
  update `0x9B68`, see "Deferred event dispatch"), run by the mode stream
  `0x92BE`: counter `0x91BA` reset, record 16 (CD track 19), the texts
  "CHOOSE LEFT RIGHT" and "SELECT WITH RETURN", a wait of 3 s, the
  chooser, an eject from hole `0x4C50`, record 2 (the chosen tune's
  closing record), a display, unblock `0x9054`. The stream is queued by
  slot-15 record `0x927A` (handler `0x11`), which the hole's stream
  `0x4C88` takes when it is lit.
- Start: the player's choice 0, the timers set, state `+0xF1` (a copy of
  the last scan code, written by the keyboard handler with `LAST_KEY`)
  cleared. Update, every call: the left flipper (state `+0x2A7B`) lowers
  the choice, the right one (`+0x2A7C`) raises it, within 0..2, at most
  one step per `0x19` calls; the choice's name drawn through the host
  vector's `+0x14`; when state `+0xF1` is `0x1C` (Enter) the choice's
  template is copied over records 0..2, the event stream `0x4CBE` (a take
  of `0x5128`) queued through `+0x1C`, and the update returns with ZF
  clear, which ends it.
- Record 12 (`FFFE`, order `0x1E`, track 15) is in no template and has
  no pointer to it: track 15 is not played by table 2's data.

What the main image does with records 0..2 afterwards: record 0 is
header slot 34 itself, requested at `0x2B1DC` and `0x2BB3E` (at each
ball's serve, presumably: in a run at the game's start and twice later); record 1 is requested by the streams of header slots 27 and
28; record 2 closes most of table 2's modes.

## Table 1's shooting game

Found 2026-09-29 from table 1's module (a throwaway capstone script with
the relocations marked) and seen in runs the same day (docs/HANDOFF.md,
"Table 1's shooting game in a run"). The opcode-`0x14` object at
`0x9A46` (start `0x9A4E`, update `0x9B25`) is a game on the dot-matrix
display (160 x 16 dots): bad guys show up in four windows of a street,
the player moves a crosshair between them with the flippers, and one
in the crosshair's window is shot when it aims.

- It is run by mode stream `0x5EA8` (the mode of counter `0x421E`'s
  threshold 5, stream `0x4326`): blocks `0x41E6`, `0x4362`, `0x4492`,
  `0x51E0`, takes `0x417E`, shows display `0x5F18` ("GIMME YOUR BEST
  SHOT" / "TO CLEAR THE STREET", 5 s), waits, plays `0x11E49`, runs the
  game (`14 @9A46`), plays `0x11E0D`, ejects the ball from hole
  `0x4F18`, plays `0x11DA1`, unblocks the four records and takes
  `0x41E6`.
- The module code keeps its values in the dwords at `[0]`..`[0x3C]` of
  its data segment (a compiler's registers, presumably) and pointers
  into the data point at a field's end (the BCD adds work down from
  them); the addresses below are the fields' starts.
- The state record at `0xA19F`: `+0` hits (word), `+2` lives (word, 4 at
  the start), `+4` the crosshair's window 0..3, `+6`/`+7` the left and
  right flipper's repeat counts, `+8` `0xFF` from the 25th hit, `+0x0A`
  .. `+0x11` the game's score (8 packed-BCD bytes, the four at `+0x0E`
  the low ones), from `+0x12` four windows of 6 bytes (a timer in
  frames, a step 0..9, a look 0..3), `+0x42` the index into the
  delays.
- Start: hits 0, lives 4, window 1, score 0; each window's timer from
  the next of the words at `0xA227` (`+0x42` counts on, modulo 256;
  the first sixteen are 32, 161, 48, 17, 160, 145, 16, 161, 160, 1, 0,
  177, 160, 49, 32, 145), step and look 0.
- Update, once a frame: the street (`0xA0F9`, the whole display);
  each window: its timer counted down; at 0 the step goes on and the
  timer and look come from the pairs at `0xA1F3`: step 1 (25 frames,
  look 1), 2 (25, look 2), then 3 frames each: looks 3, 2, 3, 2, 3, 2,
  3; step 5's look has bit 15 set: the bad guy shoots, lives - 1, audio
  record `0x11CA9`. After step 9 the window is empty again (step and
  look 0, a new delay). A window with look 2 whose number is the
  crosshair's is hit: hits + 1, audio record `0x11CC3`, the score +
  1,000,000 (the BCD at `0xA43F`), the window emptied. Otherwise the
  window's picture (number 1 + 3 x window + look - 1, at x 16, 48, 80,
  128 from `0xA1E3`, 32 x 16) is drawn. Then the flippers: the left one
  moves the crosshair a window left, the right one a window right, at
  once and then every 25 frames while held (`+6`/`+7`); the crosshair
  (picture 14, 16 x 15, transparent, `0xA142`) at the display offset
  from `0xA1EB` / 2.
- The end: lives at 0: the hits written into "YOU SHOT 00 BAD GUYS"
  (`0x6050`); with 25 hits or more (`+8`) stream `0x5F96` is queued (it
  lights slot-15 record `0x5FC6`, handler 1: the extra ball), else
  `0x5FA2` (the text and the game's score for 2 s). The score is not
  paid on this path (read, not checked in a run). 30 hits: the score +
  50,000,000 (the BCD at `0xA437`), the whole score added to the current
  player's (state `+0xD76`'s record `+0`), stream `0x5FAE` queued
  ("EXCELLENT" with the palette flashing, the score, "EXTRA BALL IS
  LIT" five times, record `0x5FC6` lit). Either way the update returns
  with ZF clear, which ends the object.
- The pictures are presumably `data\s001\special\vm_*` (the names at
  `0xA447`, 15 of them: `vm_001`, then `vm_002a`..`vm_005c`, four times
  three as the windows and looks are, then `vm_006a`, `vm_006b`),
  loaded by the host into the table at FS; which file is which number
  is not checked.

## Table 4's sea game

Found 2026-09-29 from table 4's module (a throwaway capstone script with
the relocations marked) and runs (docs/HANDOFF.md, "Table 4's sea game
in a run"). The opcode-`0x14` object at `0x97E5` (start `0x97ED`, update
`0x9878`, code up to about `0xB1F3`) is a boat on the display that the
flippers steer past rocks, picking up bonuses. Not read: the drawing
routines (`0xA16E`, `0xA3EC`, `0xAF4D`, `0xB0FA` and what they call).

- It is run by mode stream `0x8F72` (the mode of counter `0x5F60`'s
  threshold 2, stream `0x6008`): music `0x195D5`, display `0x9022`
  ("CONQUERING THE SEA", then "AVOID THE ROCKS AND" / "COLLECT THE
  BONUSES"), music `0x19611`, the game, music `0x19581`, an eject from
  hole `0x460C`, takes `0x5F28` and `0x61C8`. Counter `0x5F60` is
  counted up by record `0x5F28` (handler `0x15`, taken by header slot
  27's stream at the serve and by this mode); record `0x5EA2` (handler
  `0x16`) starts the threshold's mode when zone `0x33FF`'s stream
  (`0x4644`) takes it lit, as table 1's `0x4362` does.
- State (bytes unless said): `0xACB6` (word) which arrow to draw, see
  below; `0xACB8` `0xFF` while a bonus is on the water; `0xACB9` column
  7's object as it was before the last row; `0xACBA`..`0xACC1` the eight
  columns, one object each: the high nibble its kind (0 a rock, 2 a
  5,000,000 bonus, 4 a 10,000,000 bonus, 6 the extra ball), the low
  nibble its row 1..15 down the water, 0 an empty column; `0xACC2`..
  `0xACC9` held a copy of them in a run (who writes it not looked at);
  `0xACCA` a frame count; `0xACCB` frames of immunity; `0xACCC` the
  speed; `0xACCD` the running move's step (signed); `0xACCE` (dword)
  the row count; `0xACD0` (word) the scroll, modulo `0x200`; `0xACD2`
  the boat's lane 0..7; `0xACD3` its position 0..`0x7F` in a move;
  `0xACD4` the index into the course at `0xACFA`; `0xACD5` the course
  step's bonus kind; `0xACD6` (word) the random number; `0xACD8` Enter
  used (`0xFF`); `0xACD9` frames to the next row; `0xACE4` the end
  flag; `0xACE5` and `0xACE6` a running script (a frame count and a
  pointer).
- Start: the 28 bytes from `0xACBA` cleared (the columns up to
  `0xACD5`: lane 0, course step 0), `0xACD8`, `0xACB8`, `0xACE4` 0,
  `0xACD9` 1, the random number stirred; the host vector kept for the
  update. `0xACD6` is not cleared. State `+0xF1` (the last key) is not
  cleared either, so an Enter still there from the serve counts at once
  (seen in a run).
- The random number (`0xA27B`, at the start and every frame): `0xACD6`
  plus the PIT's counter 0 (latched and read at port `0x40`), then
  `ADC` of itself, `XOR` the word at `0xACBA`, plus the row count. So
  the rocks come from the timer; in the runner, whose PIT keeps the
  emulated clock, a run repeats.
- Update, once a frame: while a script runs, its next step (`0x9A20`:
  a picture a step, the script's routine at the end); else Enter
  (state `+0xF1` = `0x1C`) the first time: script `0x9A88` (24 frames a
  step), audio record `0x1953D`; else the frame: immunity counted down,
  the frame count up, the scroll moved by half the speed, the random
  number stirred, the steering (`0xA2E1`), the bonus check (`0xA067`),
  and when `0xACD9` runs out a row (`0x9C91`) with the course step's
  values; then the drawing. The update ends the object (ZF clear) when
  `0xACE4` is set.
- The course, 4 bytes a step: the frames to the next row, the speed,
  the bonus kind, a fourth byte not read. Steps 0..2: 4 frames, speed
  4, kind 0; 3..7: 3 frames, speed 4, kind 0; 8..14: 2 frames, speed 8,
  kind `0x20`; 15: 2 frames, speed 8, kind `0x40`. The index goes on a
  step (+4) only when a 5,000,000 or 10,000,000 bonus is collected, so
  the game gets faster by bonuses, not by time: eight bonuses of
  5,000,000, seven of 10,000,000, then the extra ball; 110,000,000 at
  most.
- A row (`0x9C91`): each column in turn (`0xACBA` up). An object moves
  a row down, from rows 1..7 only on even row counts, from 8 on every
  row. One that leaves row 15 is cleared; when its column is the
  boat's, it meets the boat first (below). An empty column gets a new
  object when the random number's low three bits are 0 (one in eight)
  and the column before it (column 7 before the row, for column 0) is
  empty or at row 1 or 2: a bonus of the step's kind at row 1 (`0x21` +
  `0xACD5`) if none is on the water and bits 12..13 of the random
  number are not both 0, else a rock at row 1, but no rock while
  immunity runs.
- The boat's column: lane + 1, rounded (+1 more when the position is
  `0x40` or more), modulo 8.
- Meeting the boat: a jump through the table read with BX at `0x9EBF`
  + (the value + 1) / 16, i.e. the words at `0x9EC0` by the kind: rock
  `0x9EC8`, the crash: script `0x9AEC` (`0x36` frames a step), audio
  record `0x19557`, and at its end `0x9C44`; kind 2 `0x9F68` and kind 4
  `0xA053`: the player's score + 5,000,000 (BCD at `0xAD42`) or +
  10,000,000 (`0xAD4A`), audio record `0x194A1`, the course index + 4;
  kind 6 `0x9F1D`: stream `0x90AE` queued (a take of slot-15 record
  `0x90CC`, handler 1: the extra ball) and the end flag set. Odd kinds
  would read no code target; the row code makes none. The crash does
  not look at immunity: a rock already on the water crashes the boat
  (an earlier note here said otherwise).
- Steering (`0xA2E1`), only when no move runs (`0xACCD` 0 and the
  position 0): the left flipper (state `+0x2A7B`, checked first) takes
  the lane down by one at once and the position from `0x80` - speed
  down by the speed a frame; the right one (`+0x2A7C`) takes the
  position up from 0 by the speed a frame and the lane up by one when it
  reaches `0x80`. The lane wraps modulo 8. A move takes `0x80` / speed
  frames (32 at speed 4, 16 at 8) and runs to its end whatever the
  keys; a held flipper starts the next at once.
- The bonus check (`0xA067`): `0xACB8` set while one of `0xACB9`..
  `0xACC0` has a high nibble, else cleared with `0xACB6`. With a bonus,
  `0xACB6` is 0 when it is in the boat's column, else 6 or 18 by side,
  6 more by the distance round (not worked out further); the drawing
  (`0xA9C7`) then shows the picture of the 6-byte entry at `0xAC98` +
  `0xACB6` on three frames of four, presumably an arrow to the bonus
  (not looked at on the screen).
- Enter's script ends in `0x9BC8`: every object whose high nibble is 0
  (the rocks) cleared, 150 frames of immunity. Once a game.
- `0x9C44`, the crash's end: stream `0x90BA` queued (music `0x196DD`,
  display `0x8FC6`: a picture, "ITEM" / "COLLECTED" / "FISH", 2 s), and
  the update ends the object. So the game ends in "ITEM COLLECTED:
  FISH" on a crash, in "EXTRA BALL" after the 15 bonuses.

## Initial shared slots

The copied-header scan establishes the following host-visible groups and the
module-local roots whose roles are already known. A main address is the copied
slot address, even when the instruction reaches it through runtime state.

| Slot | Main address | Observed use |
| ---: | ---: | --- |
| 0-5 | `0xF3E4`-`0xF3F8` | First bundle of six pointers. `0x2C414` copies them into a ball's `+0x50`..`+0x64` and clears its byte `+8`: zone type 3 does this (checked 2026-09-29 in the hints), and a hole puts a ball out with it or the other bundle by its word `+0x0E` (`0x30D78`). So the two bundles are two levels a ball can be on, presumably; "flipper" for the whole bundle is not checked. Slot 5 is a list of 14-byte zones (x0, y0, x1, y1, type, relocated object; `-1` ends it), checked against each ball at `0x2C1DA`; the types are in the hints (`0x2C3DD`). |
| 6-11 | `0xF3FC`-`0xF410` | Second bundle, copied by `0x2C467` (byte `+8` set to `0xFF`), by zone type 2 and when a ball is placed at `0x29A5A`. Slot 11 is a second zone list like slot 5's. |
| 12-13 | `0xF414`-`0xF418` | Self-sized 16-bit offset directories selecting variable-size gameplay records. |
| 14 | `0xF41C` | Null-terminated registry of light-group descriptors and linked light states. |
| 15 | `0xF420` | Null-terminated registry of fixed 52-byte timed-effect states. |
| 16 | `0xF424` | Null-terminated registry of per-player progression counters with threshold actions. |
| 17 | `0xF428` | Optional registry of cumulative-weight presentation-choice arrays. |
| 18 | `0xF42C` | Signed X/Y collision-response vectors indexed by a byte read from the stage collision map. |
| 19 | `0xF430` | Stage bitmap pointer used by the playfield setup paths at `0x91F7` and `0x924E`. |
| 20 | `0xF434` | Stage palette/data pointer used at `0x91DB` and by palette-processing code around `0xA66A`. |
| 21 | module-local | Path string `DATA\\S00x\\BALL.M`; unlike flagged assets, it remains a string after loading. |
| 22 | `0xF43C` | Pointer to four fixed-stride `0x1F7`-byte flipper records followed by a zero-type sentinel, consumed by initialization and gameplay paths. |
| 23 | `0xF440` | Optional 18-byte flipper-interaction zones, each selecting a slot-22 flipper record. |
| 24 | `0xF444` | One-byte pending-object selector consumed and cleared by the timed update at `0x2F322`. |
| 25 | `0xF448` | Pair of special gameplay-object pointers immediately following the slot-14 list. |
| 26 | `0xF44C` | Null-terminated registry of 42-byte bounded BCD accumulators. |
| 27-28 | `0xF450`-`0xF454` | Two event streams; zone type 0 (`0x2C59C`) queues one of them through `0x2FE8E` (checked 2026-09-29 in the hints). |
| 29 | module-local | Path template `DATA\\S00x\\MOD@MUSIC@P`, retained for module-side loading. |
| 30 | module-local | Path template `DATA\\S00x\\MOD@MUSIC2@P`; null for table 4, which has no second music module. |
| 31 | module-local | Base of a mutable 26-byte record bank described below. BPC command streams contain relocated aliases to its first record. |
| 32 | `0xF464` | Callable end-of-ball bonus presentation invoked through the state-relative read at `0x2BEED`. All four implementations build the `COMBOS`, `BONUS`, `TOTAL BONUS`, and `NO BONUS` displays and invoke host callbacks. |
| 33 | `0xF468` | Base of four 20-byte descriptors pointing to four contiguous 40-byte state blocks; passed through shared cell `[0]` at `0x2A6CE`. |
| 34 | `0xF46C` | Base of the mutable 12-byte audio-control record array, read at `0x2B20D` and `0x2BB4B`. |
| 35 | `0xF470` | Initial music-control record selected from the slot-34 array; fields at `+8` and `+0x0A` initialize music state at `0x9BCA`. |
| 36 | `0xF474` | Exported audio-control record read at `0x2AA96`. |
| 37 | `0xF478` | Exported audio-control record read at `0x2ABCF`. |
| 38 | `0xF47C` | Exported audio-control record read at `0x2B901`. |
| 39 | module-local | Post-load hook called by the main loader; a `ret` stub in all four modules. |
| 40 | `0xF484` | Callable transition hook invoked at `0x2AA6D`; nontrivial only for table 2. |
| 41 | `0xF488` | Callable transition hook invoked at `0x2BD61`; implemented by tables 1 and 2 and stubbed by tables 3 and 4. |
| 42 | module-local | Null in all four modules. |
| 43 | module-local | Pointer to an event opcode 1Bh (multiball) command in a mode stream (table 1 8FC8h, 3 6EA2h, 4 809Ah; table 2 points to zeros). At loading `MULTIBALL_CAP` replaces its ball count (word `+2`) by the option MULTIBALL MAXIMUM, at most 6, -, 6, 4 by table (checked in runs on table 1, 2026-09-29, see docs/HANDOFF.md). |
| 44 | module-local | As slot 43, a second multiball command (table 1 8598h, 4 8786h; tables 2 and 3 point to zeros): the option, at most 4, -, -, 6 by table. |

The host-visible layouts of the five object registries are described below.
Some module-private fields remain deliberately unnamed because no main-image
consumer establishes their meaning.

### Slots 12-18 gameplay registries

Slots 12 and 13 begin with a directory of 16-bit offsets relative to the slot
base. The first offset is also the directory byte size, so it determines the
record count; a zero word ends the directory. Slot 12 exposes 3, 3, 4, and 4
records across tables 1 through 4, while slot 13 exposes two records in every
table. The selection paths at `0x12F53` and `0x13062` index these directories,
then update the chosen variable-size record. Maintenance paths at `0x2CBD5`
and `0x2CC2A` walk the same directories.

Slots 14 through 17 are null-terminated dword lists. Every non-null entry is a
relocated pointer to module data. Their object counts are:

| Table | Slot 14 | Slot 15 | Slot 16 | Slot 17 |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 26 | 123 | 16 | 0 |
| 2 | 28 | 117 | 13 | 1 |
| 3 | 21 | 98 | 15 | 0 |
| 4 | 11 | 107 | 15 | 1 |

Main-image reset and update routines at `0x29AF9`, `0x29CE3`, `0x29FFA`, and
`0x2A0C2` traverse the four lists respectively. The pointee layouts differ,
so these are four typed object registries rather than one interchangeable list
format.

Slot 14 points to ten-byte group descriptors. Dword `+0` is a relocated first
state pointer, byte `+4` contains group flags, byte `+5` is zero in every
observed descriptor, and the unaligned dword at `+6` is an optional relocated
pointer to a queued event sequence. The first pointer begins a linked chain of
30-byte light states:

| State offset | Host-visible role |
| ---: | --- |
| `+0x00`-`+0x05` | Mutable flag/status bytes reset by `0x29AF9`; bit 3 of byte `+0` selects a status-buffer path at `0x28FF8`. |
| `+0x0C` | Optional relocated alias to the same state's coordinate pair at `+0x14`. |
| `+0x10` | Relocated next-state pointer, or null at the end of the chain. |
| `+0x14`, `+0x18` | Coordinate dwords passed to the host rendering path. |
| `+0x1C` | Light ID used to index the host light-status buffers. |

The four modules contain 26/28/21/11 groups reaching 39/42/37/48 states. No
state is shared by two groups and no chain cycles. Tables 3 and 4 each contain
six `+0x0C` self-aliases; tables 1 and 2 contain none. The four modules have
1/2/2/5 group-level event links. Each target has the queue-record header and
valid terminated command stream described under deferred event dispatch.

Slot 15 contains a fixed 52-byte host-visible timed-effect state. Reset routine
`0x29CE3`, activation paths around `0x2D3D0` and `0x2D8C2`, and update routine
`0x2E8CD` establish this partial layout:

| Offset | Host-visible role |
| ---: | --- |
| `+0x00` | Packed flags and player-mask bits. |
| `+0x04`, `+0x08` | Optional relocated primary and secondary light-state pointers. |
| `+0x0C` | Optional relocated audio-control pointer dispatched through `0x3007A`. |
| `+0x10` | Optional relocated pointer to a 26-byte slot-31-style mutable display state. |
| `+0x14`, `+0x18` | Optional relocated display streams queued through `0x2FEDB` when the record is lit (event opcode 1 or 2) and taken (opcode 5). |
| `+0x1C`, `+0x20` | Points of a take: a 12-digit packed-BCD number (high word `+0x1C`, low dword `+0x20`) added by `0x2FBBF` to the player record's `+8` (the score, presumably; checked 2026-09-29 in the hints). |
| `+0x24`, `+0x28` | Points of a take too (for example `0x05000000` at `+0x28`), added by `0x2FBBF` to the player record's `+0x10` (the bonus, presumably; this row said "the score" before, checked 2026-09-29 in the hints). |
| `+0x2C` | Handler word: one of the 28 handlers at `0x2DA0A` (0: none), called after a take; each is described in the hints (CODE:2E2E1 ... 2E7FC) and docs/HANDOFF.md, "The slot-15 handlers". |
| `+0x2E` | Signed countdown, reset to `-1` and set from a duration scaled by runtime state. |
| `+0x30` | Runtime active-list next pointer. |
| `+0x34` | Present in longer records, by the handler: a slot-16 counter (handlers 6, 7, 0Ah, 0Bh, 0Eh, 0Fh, 10h, 12h, 14h..16h, 18h, 1Bh), a slot-26 BCD counter (13h), an event stream (11h), a list of 8-byte random-award entries (1Ah), a word (5: the bonus multiplier; 17h: two words); handlers 1, 2 and 8 have no `+0x34` of their own (handler 2 reads it all the same, the next record's first word) (checked 2026-09-29 in the hints and the four modules). |
| `+0x38`.. | By the handler: a word (0Ah, 14h), a 12-digit number `+0x38`/`+0x3C` (0Fh, 19h), a pointer to a slot-26 counter `+2` (1Bh). |

Only `+0x04`, `+0x08`, `+0x0C`, `+0x10`, `+0x14`, and `+0x18` carry file
relocations within the fixed state, and every non-null on-disk value in those
fields is relocated. The `+0x10` field is populated by 9/8/0/0 records across
the four modules. Its targets either belong to the slot-31 bank or use the
same 26-byte layout immediately after that bank; their optional payload pointer
at `+0x16` obeys the same relocation rule. Data observed beyond `+0x33` is not
assigned to the generic host layout.

Slot 16 holds per-player progression counters. Reset paths at `0x29FFA` and
`0x2A113`, command handlers from `0x2DF56` through `0x2E7FC`, and the BCD-add
routine at `0x2FD11` establish the following host-visible fields:

| Offset | Host-visible role |
| ---: | --- |
| `+0x00` | Flags word; host paths test bits 1 and 2. |
| `+0x02` | Initial word. |
| `+0x06`, `+0x16` | Two arrays of eight per-player words. |
| `+0x26` | A timer in frames: set by slot-15 handler 14h (seconds times the frame rate), counted down each frame of play by `0x2CB69`; at 0 the step goes back to `+0x28`/`+0x2C` and the value to 0 (checked 2026-09-29 in a run, table 1). |
| `+0x28`, `+0x2C` | Configured dwords copied into runtime dwords at `+0x30`, `+0x34`. |
| `+0x30`, `+0x34` | The step: a 12-digit packed-BCD number (high word, low dword), raised by slot-15 handlers 0Fh and 1Bh, lowered by 19h, paid by 0Eh (checked 2026-09-29 in the hints). |
| `+0x38` | Six-byte runtime BCD accumulator in an eight-byte field: the value (high word `+0x38`, low dword `+0x3C`), raised by the step (handler 0Bh), paid by 7 and 0Ah. |
| `+0x40` | Six-byte configured BCD value, commonly the all-`FF` sentinel, in an eight-byte field: the value's cap, presumably (handler 0Bh; a negative `+0x40` dword, the `FF` sentinel, no cap). |
| `+0x48`, `+0x4C` | Optional relocated queued-event pointers. |
| `+0x50` | Start of the threshold-action run. |

Each 12-byte threshold action contains an increasing signed threshold at `+0`,
a per-player fired/state byte at `+2`, a mandatory relocated presentation
pointer at `+4`, and an optional relocated light-state pointer at `+8`. A
negative threshold (`-1` or `-2` in the installed data) terminates the run.
The optional event fields select 3/0/1/3 sequences across the four modules;
their targets have a four-byte queue header followed by a valid terminated
command stream.

Slot 17 is empty in tables 1 and 3 and contains one array in tables 2 and 4.
Each eight-byte choice consists of a flags word, a strictly increasing
cumulative limit, and a relocated presentation pointer. The host selector at
`0x2E51D` uses flag bits 0 and 1 for repeat/used state. Both arrays have eight
choices and end at `0x100`; their limits are respectively
`25,60,105,111,146,186,231,256` and
`32,64,96,128,160,192,224,256`.

Slot 18 is a packed array of signed 16-bit X/Y pairs. At `0x134D9`, a byte
from the stage collision map indexes the array with scale four; the chosen pair
is added to the current coordinates. The array ends exactly where the slot-22
flipper bank begins and contains 15, 13, 14, and 10 vectors across the four
tables. `bpc_inspect.py` validates and reports all of these container layouts.

### Slot-22 flipper records

Slot 22 is an active main/module boundary. Every table supplies four records
at stride `0x1F7`, immediately followed by a record whose first byte is zero.
The main initializer at `0xB8A8` uses that byte as the terminator, skips records
whose type byte is 3, and advances by exactly `0x1F7` after every record. The
observed type sequences are `1,3,1,1` for tables 1 and 4 and `3,1,1,1` for
tables 2 and 3.

For non-type-3 records, the initializer walks word `+6` toward word `+8` in
the direction selected by word `+0x0A`, wrapping modulo 120. It counts those
steps in word `+0x14`, scales the count by 64, subtracts one, clears word
`+0x12`, and conditionally negates the resulting fields for the reverse
direction. Words `+2` and `+4` form screen-coordinate pairs, and the sole
relocated field within every active record, dword `+0x1C`, aliases `MASK1` or
`MASK2`. Together with the two six-slot flipper bundles, this identifies the
bank as three active flippers plus a type-3 placeholder. The complete
`0x1F7`-byte record layout remains provisional. `bpc_inspect.py` validates
the four-record shape, empty placeholder, angle bounds, trailing active marker,
and the sole per-flipper relocation to `MASK1` or `MASK2`; its report lists the
pivot, angle endpoints, direction, and selected mask for every active record.

### Slots 23-28 auxiliary gameplay exports

Slot 23 is a run of 18-byte records terminated by a signed `-1` word. Tables 1
and 2 contain one record; tables 3 and 4 point directly at the terminator. Each
record holds four boundary words, a second two-word range, a byte selector,
and a relocated pointer at `+0x0E` to one of the slot-22 flipper records. The
consumer at `0x116D7` compares all three ranges and the selector against live
ball state; a match clears word `+0x10` in the selected flipper. These are
therefore flipper-interaction zones, although the exact physical condition
represented by the third range remains provisional.

Slot 24 points to a byte initially set to zero. One additional BPC relocation
aliases it in every table, allowing module data or commands to arm the value.
The main update at `0x2F322` consumes and clears a nonzero value, uses it to
index a runtime object-pointer array, and subtracts `0x1770` from the chosen
object's word at `+0x10`. It is best treated as a pending-object selector until
the producer is decoded.

Slot 25 immediately follows the zero terminator of the slot-14 object list and
contains two relocated object pointers; the second is null in table 2. Main
paths at `0x2B889`, `0x2B928`, and `0x2BCF5` read these two pointers and alter
small state bytes in their pointees during timed gameplay transitions.

Slot 26 is another null-terminated relocated object-pointer list with 2, 1, 9,
and 1 entries across the four tables. Every pointee is a 42-byte mutable
packed-BCD state with no internal file relocations. Each numeric field is a
12-digit value split into a padded high word and a low dword:

| Offset | Host-visible role |
| ---: | --- |
| `+0x00` | Active byte. |
| `+0x01` | Flags byte; bit 0 selects addition versus subtraction. |
| `+0x02`, `+0x06` | Current packed-BCD value. |
| `+0x0A`, `+0x0E` | Configured starting value. |
| `+0x12`, `+0x16` | Limiting value. |
| `+0x1A`, `+0x1E` | Per-update step. |
| `+0x22` | Eight-byte module-private trailer; only its first word varies in the installed data. |

Event opcode `0x0F`, handled at `0x2DB5C`, copies the configured start into the
current value and sets the active byte. The update loop at `0x2EACF` then uses
`DAA`/`DAS` to move the current value by the step, compares it with the limit,
copies the limit on completion, and clears the active byte. Opcode `0x10`
clears the active byte directly. No host consumer or direct relocated module
reference gives the eight-byte trailer a narrower role. Slots 27 and 28 select
table-local presentation records: the path at `0x2C5D8` chooses one according
to current runtime state and submits it to queue routine `0x2FE8E`.
`bpc_inspect.py` validates the slot-14 through slot-17 record structure,
slot-23 zones, the slot-25 pointer pair, and all four padded BCD values in each
slot-26 record in addition to reporting them.

### Slot-31 mutable record bank

Slot 31 starts a contiguous run of 26-byte records. The observed table counts
are 8, 15, 10, and 9. Each record has the structural form:

```c
struct slot31_record {
    uint16_t tag;             /* always 2 */
    uint16_t words[10];
    void *data;               /* relocated field at +0x16 */
};
```

Words 1 and 2 range like display dimensions (for example `80, 64` and
`100, 44`), while word 3 ranges from 8 through 27 like a frame count. These
names remain provisional because the pointed payload encoding is not yet
decoded. Words 6 through 10 are consistently `1, 0, 0, 0, 0` in every record.
The run ends at the first non-tag-2 structure.

This bank is live module data rather than a header-only compatibility export.
Relocated aliases to its first record occur 21, 3, 8, and 9 times outside the
header in tables 1 through 4. Several lie directly in display
streams as the operand of display opcode `0x10`, which hands it to
`0x3007A` (type 2). Event opcode `0x10`, which this paragraph named
before, takes slot-26 records instead (checked 2026-09-29 with
tools/event_streams.py on the four modules).

### Slot-33 descriptor bank

Slot 33 points to exactly four 20-byte descriptors in every module. Their
first four words are respectively `(0,128,0,1)`, `(0,7,1,1)`, `(0,7,1,1)`,
and `(0,7,0,1)`; a relocated dword at `+8` points to a 40-byte state block,
and the remaining four words are zero. The four state blocks are contiguous.
All words in each block are initially zero except offset `+0x10`, which holds
`0x100`, `0x200`, `0x300`, or `0x400`, and offset `+0x24`, which holds ID 100,
101, 102, or 103.

The descriptor-bank base occurs only in header slot 33's own relocation in all
four BPCs, but it is an active host-facing export: the main path at `0x2A6CE`
loads it from copied state, places it in shared cell `[0]`, and calls `0x2FEDB`.
Its intended semantic role cannot yet be named more narrowly without tracing
that consumer and the four mutable state blocks.

## Host callback vector

The table modules and the main image share a low-address scratch area. In BPC
code, address-size-prefixed loads from `[0x10]` obtain a pointer to an
11-dword host callback vector. This is separate from BPC header slot 4, even
though both occupy offset `0x10` in their respective address spaces. Main-image
paths at `0x2BEDC`, `0x2D04D`, and `0x2DBD3` install vector `0x2CD10` in that
scratch cell immediately before dispatching module work.

The vector is:

| Offset | Main target | Statically observed behavior |
| ---: | ---: | --- |
| `+0x00` | `0x26A6D` | No-op entry; no BPC call site was found. |
| `+0x04` | `0x3007A` | Dispatch an audio-control record supplied through shared pointer `[0]`. The low three bits of its first byte select one of eight paths. |
| `+0x08` | `0x2FBA8` | Clear two 5,120-byte display buffers through `0x27259` and `0x27CEC`, filling them with `0x00` and `0xFC` respectively. |
| `+0x0C` | `0x2C037` | Run a timed update loop. The duration comes from shared word `[0x20]` and is scaled by runtime state at `+0x50`. |
| `+0x10` | `0x275E4` | Build a numeric/score presentation using the pointer and layout arguments in the shared scratch cells. |
| `+0x14` | `0x27783` | Render the display-text record supplied through `[0]`; the record selects layout and contains or references its text. |
| `+0x18` | `0x30368` | Convert the value in `[0x20]` to decimal ASCII, writing backwards from the end pointer in `[0]`. |
| `+0x1C` | `0x2FE8E` | `EVENT_QUEUE`: the event stream at `[0]` into the 64-entry ring at runtime state `+0x2A1E` (an event stream, not a display record: checked 2026-09-29, table 1's three and table 2's one argument parse as event streams). |
| `+0x20` | `0xBAB6` | Program VGA DAC entries `0xFC..0xFF` from the second 12-byte color set. No BPC call site was found. |
| `+0x24` | `0xBA9B` | Program the same VGA DAC entries from the first color set. No BPC call site was found. |
| `+0x28` | `0xB1B9` | Load the `ES` video selector and `FS` graphics-data selector used by following direct blits. |

A whole-image scan of the shared-cell load/call pattern finds the following
calls through this vector. The two palette entries are unused by all four
modules; offsets `+0x1C` and `+0x28` are table-dependent.

| Table | `+04` | `+08` | `+0C` | `+10` | `+14` | `+18` | `+1C` | `+28` | Total |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 3 | 4 | 4 | 3 | 6 | 2 | 3 | 2 | 27 |
| 2 | 1 | 4 | 4 | 3 | 7 | 1 | 1 | 1 | 22 |
| 3 | 1 | 3 | 4 | 3 | 6 | 1 | 0 | 0 | 18 |
| 4 | 4 | 3 | 4 | 3 | 6 | 1 | 2 | 6 | 29 |

Header slot 32's end-of-ball bonus routine exercises the common presentation
subset in all four modules. It passes a 12-byte audio-control record to
`+0x04`, brackets repeated display work with `+0x08`, submits numeric and text
records through `+0x10` and `+0x14`, and advances the sequence with `+0x0C`.
This ties the callback meanings to both the main targets and four independent
module implementations rather than to names inferred from one table.

This was found with an earlier scanner (`bpc_abi.py`) that is no longer in the
repository (it is in build/earlier/ on the machine it was removed on);
to be checked again in the hints once the 32-bit stage 1 exists.

## Deferred event dispatch

The main image fixes its runtime-state base at `0xCB3E`. This resolves an
otherwise misleading indirect call at `0x2BEDC`: state offset `+0x2926` is
absolute address `0xF464`, exactly copied-header address `0xF3E4` plus
`32 * 4`. The target is therefore header slot 32's end-of-ball bonus export,
not a separate module entry point.

Table-driven event interpretation begins at `0x2D080`. A 32-entry descriptor
table at `0x2D18F` supplies a signed handler displacement and command size for
each 16-bit opcode. Opcode `0x14` has a six-byte command and dispatches through
handler `0x2DBAD`; its four-byte operand is a relocated BPC pointer to a
two-function callback object.

A queued event record begins with two words. The host clears the runtime cursor
at `+2` when it dequeues the record, then interprets commands starting at `+4`.
Each nonzero opcode advances the cursor by its descriptor-table size; a zero
word terminates the sequence. The optional pointers in slot-14 group field
`+6` and slot-16 fields `+0x48/+0x4C` all select records of this form. Across
the four installed modules those fields reach 17 distinct sequences, including
one empty sequence and otherwise 1-6 commands per sequence. `bpc_inspect.py`
uses the 32 command sizes to reject out-of-range opcodes, truncated commands,
and unterminated sequences.

The handler stores that object at runtime state `+0x2A70` (absolute
`0xF5AE`), installs the host vector at shared `[0x10]`, and invokes the first
object function indirectly by pushing `[object+0]` and returning to it. The
idle scheduler at `0x2CFC8` later loads the same pending object, reinstalls the
host vector, and calls `[object+4]`. The update function's flags determine
whether the object remains pending or is cleared. Thus the two fields are a
start method and a deferred update method, not data pointers.

Exactly one relocation-backed opcode-`0x14` operand occurs in each observed
module:

| Table | Command | Object | Start (`+0`) | Update (`+4`) |
| ---: | ---: | ---: | ---: | ---: |
| 1 | `0x5EE0` | `0x9A46` | `0x9A4E` | `0x9B25` |
| 2 | `0x92DE` | `0x9A08` | `0x9A10` | `0x9B68` |
| 3 | `0x8E94` | `0x8EA0` | null | null |
| 4 | `0x8F9C` | `0x97E5` | `0x97ED` | `0x9878` |

Tables 1, 2, and 4 relocate both method fields. Table 3's object is a shared
zero-filled placeholder referenced by other table data, so the static
opcode-`0x14` record must not be treated as an executable callback without
additional reachability or runtime evidence. The table-4 start method also
demonstrates the ABI boundary directly: it saves shared `[0x10]` for its update
path, whose nested host calls temporarily restore that saved vector.

This was found with an earlier scanner (`bpc_events.py`) that is no longer in the
repository (it is in build/earlier/ on the machine it was removed on);
to be checked again in the hints once the 32-bit stage 1 exists.

Checked 2026-09-29 in the hints: every opcode of this table and of the
display streams' table at `0x2F621` (streams queued through `0x2FEDB`:
a flags word, two priority bytes, the position word at `+4`, commands
from `+6`), one comment per handler, and the queue at `+0x2A1E` is the
event queue, not a display one. `tools/event_streams.py` lists every
stream of a module with the roots it hangs from; docs/HANDOFF.md, "The
event language", has the summary.

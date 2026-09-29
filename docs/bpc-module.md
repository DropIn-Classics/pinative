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
at `0x9D83`, while its byte at `+0x0A` is stored at `0xCB97`. The exact meaning
of all six fields remains provisional.

The array is mutable runtime state in table 2. Routines at `0x9A55`, `0x9ADA`,
and the reset path at `0x9CD9` copy 18 words (three complete records) from one
of three templates over records 0 through 2. The template is selected through
table-local state, so a compatibility implementation must not treat the
slot-34 array as immutable asset data. `bpc_inspect.py` validates the record
run and exported indices and prints the five exported records.

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
| 43 | module-local | Pointer to a table-local descriptor. Startup writes the configured multiball capacity to its word at `+2`, capped per table at 6, unused, 6, and 4. |
| 44 | module-local | Pointer to a second table-local descriptor. Startup writes the configured multiball capacity to its word at `+2`, capped per table at 4, unused, unused, and 6. |

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
| `+0x24`, `+0x28` | The points of a take: a packed-BCD dword at `+0x28` (for example `0x05000000`) with a high word at `+0x24`, added to the player's score through `0x2FBBF` (checked 2026-09-29 in the hints; this row said `+0x2C` before). |
| `+0x2C` | Handler word: one of the 28 handlers at `0x2DA0A` (0: none), called after a take. |
| `+0x2E` | Signed countdown, reset to `-1` and set from a duration scaled by runtime state. |
| `+0x30` | Runtime active-list next pointer. |
| `+0x34` | Present in longer records: a slot-16 counter or an event stream, by the handler the word at `+0x2C` selects from the 28 at `0x2DA0A` (checked 2026-09-29 in the hints: handler 6 counts the counter, 11h queues the stream; see docs/HANDOFF.md, "The tables and their CD tracks"). |

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
| `+0x26` | Runtime/scaled interval word. |
| `+0x28`, `+0x2C` | Configured dwords copied into runtime dwords at `+0x30`, `+0x34`. |
| `+0x38` | Six-byte runtime BCD accumulator in an eight-byte field. |
| `+0x40` | Six-byte configured BCD value, commonly the all-`FF` sentinel, in an eight-byte field. |
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
| `+0x1C` | `0x2FE8E` | Enqueue the display record from `[0]` in the 64-entry ring rooted at runtime state `+0x2A1E`. |
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

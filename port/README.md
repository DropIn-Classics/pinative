# port: Pinball Illusions in C

A native compatibility implementation requiring an installed copy of the
original game, on doskit's runtime (doskit/runtime). The game's data is
read from the player's copy at run time; none of it is in this
repository.

## State

Started 2026-09-29 from doskit's template: finds or unpacks the game's
files and shows a text screen. Since 2026-09-29 it unpacks
`ILLUSION.386` from the player's `ILLUSION.EXE` (src/archive.c, the
archive as tools/illfiles.py reads it; both unpack as pMAX does since
2026-09-29, docs/HANDOFF.md, "pMAX's decoder") and loads it into doskit's
`pmem.h` memory as pMAX does (src/image.c: linear 100F30h, selector 1Ch;
the SHA-256 of src/gen/names.h checked). Translated: ENTRY and
SETUP_ARGS (src/entry.c, src/setup.c; the configuration file and
SETSOUND.DAT through src/pmax.c, in place of pMAX's services) and
SVGA_CHECK for VGA, VGA_INIT (the "Loading" picture) and HISCORE_INIT
(src/video.c), CHOOSER_LOAD's start and SOUND_START (src/sound.c; the
CD check in src/cd.c) and the sound driver's commands 0 and 4
(src/nosound.c, NOSOUND.SDR's commands in C, command 4 loading
`intro\MOD.INT`; the host callbacks they call in src/hostcb.c) and
CHOOSER_LOAD's eleven files, the intro's palettes, its video mode and
picture (src/intro.c) and the driver's command 6, the module player
(src/nsplay.c), its commands 1 and 0Dh (the timer, counted by
pictures: the port has no interrupts), 3 and 8, and the whole intro:
its script, the scroller after it, Esc and space to leave it, and the
chooser's start (src/chooser.c: its video mode, keyboard handler and
the captions compiled into code as the original makes them) and
CHOOSER's first calls (the backdrop's picture into video memory, the
driver's retrace routines, commands 0Eh and 0Fh, and the music's start),
and CHOOSER_WAIT: the scrolling backdrop of turning shapes, its
palettes and changes, the captions (their generated routines run by
reading their bytes) and the keys, up to the table menu's start
(CODE:505C), the table menu and the chooser's end (the driver's
commands 5 and 0Bh, everything freed): Esc ends the program with the
original's goodbye text; a table chosen is started (src/table.c: the
selector aliases, the keyboard handler set, the driver started again
with the table's two modules, command 11h) and TABLE_LOAD2's first
blocks (the hide-lights mask, vm_data.mgl, the dot-matrix display's
areas, fonts and animations: src/dotmatrix.c) and the table's code
module (src/module.c: SOURCE\T00n.BPC loaded and relocated, the
resources its header names, the options applied) and the table's
display (src/tblvga.c: VGA 360x350 or 320, unchained, FRAME_RATE, the
stage drawn), the ball and flipper records, the lights' and drop
targets' files, the flippers' blocks (src/tblinit.c) and pictures
(src/flipper.c: each flipper drawn at each angle, its changed pixels
and collision masks kept) and the multiball cap: TABLE_LOAD2 whole;
the game's first steps (src/play.c, src/lights.c: the display queue
and the lights reset, the drop targets drawn, the attract scroll's
first line, the flippers drawn, the nudges and the flippers' moves with
their sounds, src/flipper.c and the driver's commands 9 and 12h, the
lights' states and flashing a frame, the dot-matrix display shown and
the table's palette faded in) up to CODE:9B92 (`Stopped before
CODE:9B92`); the Info page (a table's
picture, text and high scores) and the greetings page in their own
256-colour mode. With -mem the port writes its memory at the program's end
as well. The driver's picture measurement (command 0Eh) is
computed from the CRTC's registers as dosrun times them.
The port loads
NOSOUND.SDR, the silent driver, whatever driver the configuration names; `-opt o`,
`s` and `r` stop at the options screen, the sound set-up and the
options' reset, `-opt ?` prints the help.
The game folder is recognised by `ILLUSION.EXE` (also what must come out
of the GOG image when it is unpacked); built with build.sh on macOS
2026-09-29, the image unpacking not tried.

## Build and run

    sh port/build.sh          # macOS, Linux (SDL2 for the window)
    port\build.bat            # Windows (MSVC)
    port/build/pinative -game game
    port/build/pinative-headless -game game -cfg ILLUSION.CFG [-opt LETTERS] -mem FILE [-vram FILE]
                              # the memory where the port stops (-entry:
                              # at ENTRY), for memcmp.py --base 100F30
                              # (--vram with dosrun's -vram)

Print Screen writes the picture shown into the next free
`screenshot_NNNN.png` in the current folder (doskit's `shot.h`); the
headless build writes the pictures `DK_SHOTS` names, as
`DK_SHOTS="150:build/a.png 299:build/b.png"`.

Both scripts define `PORT_VERSION` (a string) for the compiler when
there is a version: the environment's `PORT_VERSION`, else the tag of the
commit built; the workflow sets it for a tag's build. Without one it
stays undefined; nothing in the port reads it yet.
`PORT_UPDATE_URL` likewise, from the environment only: where a release
looks for newer ones (doskit/runtime/update.h); the workflow sets it to
the latest release's `latest.json`. Nothing reads it yet either: the
port has no setup screen of its own to ask on (RELEASE.md point 7).

The runtime is in the doskit submodule: after a clone or a pull,
`git submodule update --init` (the build fails with files not found
otherwise). Any `ILLUSION.CFG` the game's set-up wrote will do for
`-cfg`, whatever driver it names: the port always loads NOSOUND.SDR
(only dosrun's comparison runs need a file naming it). Without `-cfg`
the port stops at SOUND_SETUP; a file with an SVGA mode stops it at
SVGA_CHECK.

## Releases

`.github/workflows/build.yml` (doskit's template's since doskit e06b74d)
builds the packages doskit/docs/RELEASE.md prescribes; a pushed tag
`vX.Y` makes a release of them. `dist/README.txt` is the players' README
in each package, filled in for the chooser the port reaches so far.

No release made yet; no package started from a download. The new
workflow not run yet (the macOS app, its static SDL2, the checks).

## Checked

- 2026-09-29, Linux (gcc 14.2): the memory `-entry -mem` writes against
  dosrun's `-mem` at ENTRY (linear 1011D3h; docs/HANDOFF.md, "The image
  in memory at its entry"): `memcmp.py src/ILLUSION.hints A B --base
  100F30`, CODE and TAIL 0 bytes differ.
- 2026-09-29, Linux: src/archive.c's unpacking of all 125 entries equal
  to tools/illfiles.py's `extract --all` (a scratch program, not in the
  repository), 0.9 s against 66 s.
- 2026-09-29, Linux: at SETUP_ARGS (dosrun `-break 133E69 -mem`), the
  GOG `ILLUSION.CFG` and a crafted one with 200h random option bytes
  (docs/HANDOFF.md, "The configuration file's options"): CODE and TAIL
  0 bytes differ.
- 2026-09-29, Linux: at SVGA_CHECK (dosrun `-break 101683 -mem`), the
  GOG `ILLUSION.CFG`: CODE and TAIL 0 bytes differ, and SETSOUND.DAT at
  the same linear address (1473C0h) in both; `-opt ?` the original's
  help text (docs/HANDOFF.md, "SETUP_ARGS in a run").
- 2026-09-29, Linux: at VGA_INIT's first call (dosrun `-break
  1013CA#1 -mem`): CODE and TAIL 0 bytes differ.
- 2026-09-29, Linux: at CODE:757D (dosrun `-break 1084AD#1 -mem
  -vram`): CODE, TAIL and video memory 0 bytes differ.
- 2026-09-29, Linux: at the driver's command 0 (dosrun `-break
  10804D#1 -mem`, NOSOUND.SDR in the configuration header): CODE and
  TAIL 0 bytes differ, and the driver's block at 1473C0h (memcmp.py
  src/NOSOUND.hints ... --base 1473C0) 0 bytes differ.
- 2026-09-29, Linux: after the driver's command 0 (dosrun `-break
  108054#1 -mem`, NOSOUND.SDR): CODE, TAIL and the driver's block 0
  bytes differ, and the three blocks it allocates (800h bytes at
  14A8D0h, the DMA buffer at 13120h, the volume table at 14B0E0h)
  equal byte for byte.
- 2026-09-29, Linux: at the end of SOUND_START (dosrun `-break
  1084E6#1 -mem`, NOSOUND.SDR): CODE, TAIL and the driver's block 0
  bytes differ; all 32 blocks of pMAX's heap chain (the driver, its
  three blocks, the patterns and 28 samples of MOD.INT), the DMA
  buffer and MOD.INT's freed bytes at FA8060h equal byte for byte.
- 2026-09-29, Linux: src/archive.c's unpacking of all 125 entries equal
  to tools/illfiles.py's `extract --all` again after both followed
  pMAX's decoder (a scratch program), 1.7 s.
- 2026-09-29, Linux: after CHOOSER_LOAD's files (dosrun `-break
  1087B5#1 -mem`, CODE:7885, NOSOUND.SDR): CODE and TAIL 0 bytes differ;
  all 43 used blocks of pMAX's heap chain equal byte for byte.
- 2026-09-29, Linux: at CODE:78D9 (dosrun `-break 108809#1 -mem`,
  NOSOUND.SDR), INTRO_PALS made: CODE and TAIL 0 bytes differ, the 43
  blocks equal.
- 2026-09-29, Linux: at CODE:7925 (dosrun `-break 108855#1 -mem
  -vram`, NOSOUND.SDR), after INTRO_MODE and INTROPIX_PALS: CODE, TAIL
  and video memory 0 bytes differ.
- 2026-09-29, Linux: at CODE:795F (dosrun `-break 10888F#1 -mem
  -vram`, NOSOUND.SDR), after one command 6: CODE, TAIL, video memory
  and the driver's block 0 bytes differ; the DMA buffer at 13120h and
  the 43 used heap blocks equal byte for byte (a scratch script walking
  the chain). Only the module's first row played there.
- 2026-09-30, Linux: at the intro's end, CODE:79C3 (dosrun `-break
  1088F3 -mem -vram`, NOSOUND.SDR, no key): CODE, TAIL and video memory
  0 bytes differ; 42 of the 43 used heap blocks and the DMA buffer
  equal; the driver's block differs in 24 bytes, all where the mixing
  stands in the sample clock (SAMPLE_POS, TIMER_COUNT, MIX_POS,
  MIX_LEN, the channels' positions). The keys and the window's
  pictures not checked.
- 2026-09-30, Linux: at CHOOSER, CODE:4FF9 (dosrun `-break 105F29
  -mem -vram`, NOSOUND.SDR, no key): CODE, TAIL and video memory 0
  bytes differ; 175 of 176 used heap blocks equal, the 138 caption
  routines the chooser generates among them (the driver's in the sample
  clock only). DAC and CRTC not compared; nothing on the screen yet.
- 2026-09-30, Linux: at the chooser, CODE:4CFB (dosrun `-break
  105C2B -mem -vram`, NOSOUND.SDR), no key: CODE, TAIL and video memory
  0 bytes differ, 37 of 38 used heap blocks equal (the driver's in the
  sample clock only). With space in the script (dosrun `-key 40
  space`, the port DK_KEYS 1540) and in the scroller (`-key 90 space`,
  DK_KEYS 4550): CODE differs only in INTRO_TIME, or SCROLL_POS and
  SCROLL_LEFT, the key having come at another moment; the heap as
  without a key. The window build not run for this.
- 2026-09-30, Linux: at CHOOSER_WAIT, CODE:38C6 (dosrun `-break
  1047F6 -mem -vram`, NOSOUND.SDR, no key): CODE, TAIL and video memory
  0 bytes differ; 175 of 176 used heap blocks equal; the driver's block
  in the sample clock only, its retrace numbers (VS_RETRACE .. VS2_IRQS)
  equal. The DMA buffer differs, mixed up to the intro timer's
  SAMPLE_POS, which differed already (docs/HANDOFF.md, "The chooser's
  timer"). The window build not run for this.
- 2026-09-30, Linux: in CHOOSER_WAIT, the port stopped by DK_FRAMES
  5500 and 8433 against dosrun at the 100th and 3000th CODE:38FD
  (`-break 10482D#100`, `#3000`, the second after a backdrop change):
  CODE differs only in FRAME_SPINS (the original's count of its
  polling), TAIL and video memory 0 bytes differ; at the 3000th 175 of
  176 heap blocks equal, the driver's in the sample clock. At CODE:505C
  after Enter and after Esc at t=112 (DK_KEYS at picture 5637): the
  same. The window build not run for this.
- 2026-09-30, Linux: tables 1 and 2 chosen (Enter, Down and Enter;
  dosrun `-break 10B253`, CODE:A323) and Esc to the end (`-break
  1012A1`, CODE:0371): CODE differs only in FRAME_SPINS, TAIL equal,
  video memory equal at the table; after Esc equal before the original's
  INT 10h mode 3, which the port does not model (docs/HANDOFF.md, "The
  chooser's end").
- 2026-09-30, Linux: the Info page and the greetings page against dosrun
  at FRAME_WAIT's 900th call (`-break 1034EB#900`) and at the table
  after each: CODE differs only in FRAME_SPINS and, at the table,
  VSYNC_COUNT (the original's CPU time in CUBE_DRAW_MIX, not modelled;
  docs/HANDOFF.md, "The Info and greetings pages"), TAIL and video
  memory equal.
- 2026-09-30, Linux: table 1 up to TABLE_LOAD2 (dosrun `-break 10B320
  -mem -vram`, CODE:A3F0, Enter at 112 and 115; DK_KEYS "5637:1C 5646:9C
  5815:1C 5824:9C"): CODE differs only in FRAME_SPINS, TAIL and video
  memory equal, 55 of 55 heap blocks equal but the driver's SAVED_61
  (port 61h, which dosrun makes from the emulated clock; docs/HANDOFF.md,
  "The table's start").
- 2026-09-30, Linux: table 1 up to CODE:B22D (dosrun `-break 10C15D
  -mem -vram`, t=123.79, the same keys): CODE differs only in
  FRAME_SPINS, TAIL and video memory equal; of the 67 used heap blocks
  64 equal, the driver's in SAVED_61 as before, and two the original
  does not clear ("Hidelights mask" 217 bytes, "Temp Text area" 14):
  free memory that already differed at CODE:A3F0, written by neither
  since (docs/HANDOFF.md, "The dot-matrix display's blocks").
- 2026-09-30, Linux: each of the four tables up to CODE:911F (dosrun
  `-break 10A04F -mem -vram`; Enter at 112 and 115, Down at 114 for
  table 2, 113 and 114 for 3, 113, 113.5 and 114 for 4; the port's
  DK_KEYS Enter at 5637 and 5815, Down at 5696, 5740, 5780 as many as
  needed, each released 9 pictures later; a Down at 5666 was lost): CODE
  differs only in FRAME_SPINS, TAIL and video memory equal; of the used
  heap blocks (61, 82, 61, 59) all equal but the driver's SAVED_61, the
  leftovers in the blocks taken with AH=0Ah and not cleared (see above)
  and, on table 2, 3 bytes at +2088h of a block of the driver's (host
  callback 0), which differ already at CODE:A3F0 (docs/HANDOFF.md, "The
  module's load").
- 2026-09-30, Linux: table 1 up to CODE:BA9B (dosrun `-break 10C9CB
  -mem -vram`, the keys of CODE:A3F0), with OPT_RESOLUTION 0 and 3:
  CODE differs only in FRAME_SPINS (FRAME_RATE 61 and 59 in both), TAIL
  and video memory equal, the heap blocks as at CODE:911F and 7 bytes
  past what STAGE_TO_SPOOKY writes. VGA registers and DAC not compared.
- 2026-09-30, Linux: each of the four tables up to CODE:28C45 (dosrun
  `-break 129B75 -mem`, the keys of CODE:911F): CODE differs only in
  FRAME_SPINS, TAIL equal, the heap blocks equal but the leftovers
  named above. Video memory not dumped here (BALLS_INIT writes none).
- 2026-09-30, Linux: each of the four tables up to CODE:1527F (dosrun
  `-break 1161AF -mem -vram`, the same keys): CODE differs only in
  FRAME_SPINS, TAIL and video memory equal, the heap blocks equal but
  the leftovers named above.
- 2026-09-30, Linux: each of the four tables up to MULTIBALL_CAP
  (dosrun `-break 10C013`) and the game (`-break 10C858`), `-mem
  -vram`, the same keys; table 1 also with OPT_MULTIBALL 1: CODE
  differs only in FRAME_SPINS, TAIL and video memory equal, the heap
  blocks equal but the leftovers named above.
- 2026-09-30, Linux: each of the four tables up to CODE:30114 (dosrun
  `-break 131044 -mem -vram`, the same keys): CODE differs only in
  FRAME_SPINS, TAIL and video memory equal (six drop targets drawn on
  tables 3 and 4), the heap blocks equal but the leftovers named above.
- 2026-09-30, Linux: each of the four tables up to CODE:1048B (dosrun
  `-break 1113BB -mem -vram`, the same keys): CODE differs only in
  FRAME_SPINS, TAIL and video memory equal (the flippers drawn), the
  heap blocks equal but the leftovers named above.
- 2026-09-30, Linux: each of the four tables up to LIGHTS_STEP (dosrun
  `-break 12FEAA -mem -vram`, the same keys): CODE differs only in
  FRAME_SPINS, TAIL and video memory equal, the heap blocks equal but
  the leftovers named above. Only the flippers' rest path ran: their
  move up, the nudges, the tilt count and the driver's commands 9 and
  12h (the flippers' sounds) are not checked yet.
- 2026-09-30, Linux: each of the four tables up to CODE:A654 (dosrun
  `-break 10B584 -mem -vram`) and CODE:9B92 (`-break 10AAC2`), the same
  keys: CODE differs only in FRAME_SPINS, TAIL and video memory equal,
  the heap blocks equal but the leftovers named above; table 1's screen
  at t=133.005 (`-shot`) and the headless port's picture 5898 equal
  pixel for pixel (the fade's second-last step). No light drawn, no
  group complete there: the lights' drawing, the event queue, the
  flashing and the lane change are not checked yet.
- 2026-09-30, Windows (MSVC, build.bat) by the user: built and the
  intro seen playing to its end in the window, with the user's
  Sound Blaster ILLUSION.CFG. The build's warnings not looked at.
- Not built on macOS since.

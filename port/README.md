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
the table's palette faded in, the driver's retrace routines, the
module music silenced and the table's CD track asked for, the driver's
player started: src/table.c, commands 0Ch and 1, the port's MSCDEX
taking the play and playing nothing), the game's loop and the attract
mode's first frame step (the music's countdown and command 6, the
dot-matrix display, the lights and drop targets that changed drawn,
the retrace waited, the ball taken off and drawn by the original's own
sprite routines run from the image, src/codeint.c and src/ball.c, the
drop targets that changed, the flippers, the attract scroll and the
flippers' moves; the display's queue and streams, src/display.c, the
attract mode's display record queued, its keys; display opcodes 1
and 7, the animation played into the dot-matrix display's buffer, the
opcodes that clear the display, wait, loop and set the top colours,
opcode 3's text in the display's fonts) to the attract record's end,
the high-score pages (CODE:2A6FA: an entry a page, its score drawn from
packed BCD, src/dotmatrix.c) and the attract record again, round and
round with no key; the table's keyboard handler (KBD_IRQ, src/table.c),
so F1..F8 or keypad Enter start a game (GAME_PHASE 2, CODE:2A976: the
players, balls, lights, counters, records and drop targets reset; on
table 2 `GAME_START: the module's slot 40 is not a RET`), the ball
waiting for its launch (GAME_PHASE 6, CODE:2B1DC, src/play.c: the ball
save, the music request, the scroll in play, the drop targets' and
slingshots' steps, the balls' physics in src/phys.c: each ball's edge
sampled against the level's map, the surface's normal and number, the
bounce with its friction, the move and the slope; the zones of types
0 to 3 with their points, the serve and Enter's plunger, "PLAYER n BALL
n"), play (GAME_PHASE 4, CODE:2B76E, src/play.c and src/events.c:
the event streams with the opcodes met so far, the lit records' timers
and lamps, the counters' and objects' timers, the ball save's lamp, the
M key) up to `Stopped before BALL_SAMPLE: a flipper's mask (CODE:1241E)`
when the ball first comes near a flipper; the driver's command 2 (the pause toggle) with the
PIC's mask the port now keeps for it;
Esc's question "REALLY QUIT TABLE?" (CODE:2A8AB, src/play.c), Y leaving
the table (`Stopped before TABLE: after the game (CODE:A3FF)`), another
key back to the attract mode; with no key the port stops there only when the window is closed
(`DK_FRAMES` for the headless build: `Stopped before FRAME_STEP (the
window closed; N table frames)`); the
Info page (a table's
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
- 2026-09-30, Linux: each of the four tables up to CODE:B976 (dosrun
  `-break 10C8A6 -mem -vram`, the same keys): CODE differs only in
  FRAME_SPINS, TAIL and video memory equal, the heap blocks equal but
  the leftovers named above, the DMA buffer as at CODE:9B92. The disc
  has no audio tracks there (dosrun without -cue): a play of a real
  track is not compared; DRV_TICK and DRV_FRAME not run yet.
- 2026-09-30, Linux: each of the four tables up to CODE:298C5 (dosrun
  `-break 12A7F5 -mem -vram`, the same keys), the attract mode's first
  frame step: CODE differs only in FRAME_SPINS, TAIL and video memory
  equal, the heap blocks equal but the leftovers named above; the
  driver's block also in its timer's state (SAMPLE_POS, TIMER_COUNT,
  VSYNC_PHASE: the port counts a picture's timer IRQs at once). Only
  MUSIC_UPDATE's countdown ran; of the lights one (off) was drawn a
  table, no drop target piece (DROPS_UPDATE's drawing not checked), and
  a frame whose drawing outlasts the retrace is not compared.
- 2026-09-30, Linux: each of the four tables up to CODE:2A544 (dosrun
  `-break 12B474 -mem -vram`, the same keys), the attract mode's first
  frame: CODE differs only in FRAME_SPINS (FRAME_COUNT too until the
  port gave the retrace at FRAME_STEP's end), TAIL and video memory
  equal (the ball drawn), the heap blocks equal but the leftovers named
  above (the ball's mark in the hide-lights mask equal); the driver's
  block in SAVED_61, SAMPLE_POS and TIMER_COUNT. The ball's taking off
  and DROP_MASK did not run there.
- 2026-09-30, Linux: each of the four tables up to CODE:2F9CF (dosrun
  `-break 1308FF -mem -vram`, the same keys), the attract mode's second
  frame at its first display opcode: CODE differs only in FRAME_SPINS,
  TAIL and video memory equal (the ball taken off and drawn again), the
  heap blocks equal but the leftovers named above; the driver's block in
  SAVED_61, SAMPLE_POS and TIMER_COUNT. No key pressed: ATTRACT_KEYS'
  paths with a key not checked.
- 2026-09-30, Linux: each of the four tables up to display opcode 2,
  CODE:2F7A5 (dosrun `-break 1306D5 -mem -vram`, the same keys), after
  opcodes 1 and 7 and the animation's frames: CODE differs in
  FRAME_SPINS and FRAME_COUNT (the port's 226h, the run's 225h: the
  retrace's place in the frame, the runner's timing; see docs/HANDOFF.md,
  "The attract mode's display"), TAIL and video memory
  equal, the heap blocks equal but the leftovers named above (the same
  blocks as at CODE:2F9CF). Whether ANIMS_STEP's paths for a list of
  more than one animation and for taking one off ran is not looked at.
- 2026-09-30, Linux: each of the four tables up to display opcode 3,
  CODE:2FB17 (dosrun `-break 130A47 -mem -vram`, the same keys), after
  opcode 2: as at CODE:2F7A5 (FRAME_COUNT the port's 227h, the run's
  226h). Of the opcodes added with it only 2 ran there (dosrun `-log`,
  tables 1 and 4): 0Ah, 0Dh, 0Fh, 13h, 14h, 18h and 19h are not run yet.
- 2026-09-30, Linux: each of the four tables at the second display
  opcode 3 (dosrun `-break 130A47#2 -mem -vram`, the port stopped there
  by a scratch build, the same keys), one frame after the first text:
  CODE differs only in FRAME_SPINS and FRAME_COUNT (timing), TAIL and
  video memory equal, the heap blocks equal but the leftovers named
  above (DM_TEXT equal, 357 bytes set, the 5-row font). Up to the
  high-score pages, CODE:2A6FA (dosrun `-break 12B62A`), the same, with
  11 texts drawn in the run; DM_TEXT is cleared again there, so of the
  texts only the first is compared byte for byte. Which of the fonts
  and alignments ran is not looked at.
- 2026-09-30, Linux: each of the four tables one frame after the
  first high-score page, CODE:2A7D4 (dosrun `-break 12B704 -mem -vram`,
  the port stopped there by a scratch build, the same keys), and after
  the fifth page, the attract record queued again (`-break 12B5F1#2`):
  CODE differs only in FRAME_SPINS and FRAME_COUNT (timing; the port
  one ahead), TAIL and video memory equal, the heap blocks equal but the
  leftovers named above, so DM_TEXT equal ("1 ICE" and 1,000,000,000 on
  table 1). The idle score (CODE:2758B, now translated with the same
  number drawing) did not run; a number with a nibble above 9 (CF) was
  not seen. A headless run with `DK_FRAMES=9000` ended at the window's
  close after 3098 table frames, a screenshot showed "2 ANY
  500,000,000".
- 2026-09-30, Linux: table 1, F1 in the attract mode (dosrun `-key 150
  3B`, the port's DK_KEYS `7087:3B 7096:BB`), at GAME_PHASE 2's routine
  CODE:2A976 (`-break 12B8A6 -mem -vram`): CODE differs only in
  FRAME_SPINS and FRAME_COUNT (the port one ahead; with the key at
  picture 7086 the scroll was a line behind), TAIL and video memory
  equal, the heap blocks equal but the leftovers named above; the same
  with keypad Enter (`E01C`, `7087:E0 7087:1C 7096:E0 7096:9C`). The
  dropped Space or Alt press (KEY_RELEASED 0) was not tried.
- 2026-09-30, Linux: table 1, Esc at t=150 (the port's picture 7087)
  and Y at 152 (7157; `-key 152 15`), at the main loop's end, CODE:BA7C
  (`-break 10C9AC -mem -vram`): CODE differs only in FRAME_SPINS, TAIL,
  video memory and the heap blocks equal but the leftovers named above.
  Esc and N at the same times, at the attract record queued again
  (`-break 12B5F1#2`, a scratch stop in the port): CODE differs only in
  FRAME_SPINS and FRAME_COUNT. With Space instead of N the nudge cells
  differ (state+0D44h, 0D48h and what follows from them): the run's key
  came during the question frame's drawing, after its FLIPPERS_STEP, the
  port's comes at the frame's wait (docs/HANDOFF.md, "Esc in the attract
  mode").
- 2026-09-30, Linux: each of the four tables, F1 at t=150 (dosrun `-key
  150 3B`; the port's pictures 7087, 7063, 7102, 7143 for tables 1..4,
  where FRAME_COUNT and the attract scroll came out as the run's), at
  GAME_PHASE 6's routine CODE:2B1DC (`-break 12C10C -mem -vram`; table
  2 at the slot-40 call, `-break 12B99D`): CODE differs only in
  FRAME_SPINS, TAIL and video memory equal, the heap blocks equal but
  the leftovers named above. The resets ran on a table fresh from the
  attract mode, so most of what they write was already so; a second
  game (after a game over) not tried.
- 2026-09-30, Linux: GAME_PHASE 6 on table 1 (dosrun `-key 150 3B`,
  the port's F1 at picture 7087 as before): at the frame loop's start
  CODE:2B21E (`-break 12C14E#N`, a scratch stop in the port, since
  removed) after 1, 99 and, with Enter (dosrun `-key 153.24 1C`, the
  port's DK_KEYS `7201:1C 7210:9C`), 129 frames, and at GAME_PHASE 4's
  routine CODE:2B76E (`-break 12C69E`, t=155.53, where the port stops):
  CODE differs only in FRAME_SPINS, TAIL and video memory equal, the
  heap blocks equal but the leftovers named above and the driver's
  sample clock. Tables 3 and 4 (F1 at 7102 and 7143, Enter 114 pictures
  later; dosrun Enter at 153.24): the same at CODE:2B76E. Table 2 still
  stops in GAME_START. The driver's command 2 (called twice by
  MUSIC_UPDATE at the phase's start): PAUSED, PAUSE_IMR1 (FCh) and
  PAUSE_IMR2 equal. The driver's clock drifts in this phase: a frame
  of the run takes about 170,500 instructions (16,708,861 for 98
  frames), 28.4 ms at dosrun's 6,000,000 a second, two pictures, where
  the port does a frame a picture; TICKS went up 83 in the run and 70
  in the port over those frames, so the module's position differs too
  (not audible: NOSOUND). Not reached here: a flipper's mask, the
  flippers' surfaces, bumpers' and slingshots' kicks, two balls, zone
  type 4, a light record in a zone, RECORD_DISPATCH (the port stops by
  name at each).
- 2026-09-30, Linux: GAME_PHASE 4 on table 1 (dosrun `-key 150 3B -key
  153.24 1C`, the port's F1 at picture 7087 and Enter at 7201 as
  before), at BALL_SAMPLE's flipper-mask path CODE:1241E (`-break
  11334E`, t=156.050743, where the port stops): CODE differs only in
  FRAME_SPINS, TAIL and video memory equal, the heap blocks equal but
  the leftovers named above and the driver's sample clock. Ran there
  (dosrun `-cover`): event opcodes 2, 5 and 13h, take handler 15h and
  COUNTER_LEVELS' reached threshold, LIT_LIST_STEP's countdown and
  blinking, the ball save's lamp; not reached: a lost ball, a record
  leaving the lit list, LAMP_OFF, the other opcodes (the port stops by
  name at those not translated).
- 2026-09-30, Windows (MSVC, build.bat) by the user: built and the
  intro seen playing to its end in the window, with the user's
  Sound Blaster ILLUSION.CFG. The build's warnings not looked at.
- Not built on macOS since.

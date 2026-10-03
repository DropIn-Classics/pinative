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
SVGA_CHECK (the SVGA modes as a VESA card answers, since 2026-10-01), VGA_INIT (the "Loading" picture) and HISCORE_INIT
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
M key, the flippers' masks and surfaces in src/phys.c, the bumpers'
and slingshots' kicks, the objects a ball hits (OBJECT_HITS: type 0
with its light and points, drop targets, type 2's record taken), the
holes (a ball taken, held and ejected with the hole's picture
flickering), the records' take handlers (but two no table uses), the modes
(a mode stream started, its timed waits), table 2's tunes, more of the display's
opcodes (numbers, the bonus, blinking text), the driver's jingles, a
zone's light flashed, a module's note through the driver's command 7),
a lost ball
served again under the ball save (GAME_PHASE 7, "DON'T MOVE"), a lost
ball without it (GAME_PHASE 5, CODE:2BBC6: the bonus with its
multiplier, the table module's bonus display, slot 32, and table 1's
slot 41 in C in src/modcode.c, the score paid, the next player or ball
and its resets), an extra ball (GAME_PHASE 8, CODE:2BB3E, "EXTRA
BALL"), game over (GAME_PHASE 3, CODE:2AA90: the scores against the
table's high scores, "PLAYER n GOT A HIGHSCORE" and three letters
typed, Backspace and Enter) and the players' scores and "GAME OVER"
in the attract mode after it (CODE:2A557); the driver's command 2 (the pause toggle) with the
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
At the end the port does not print the original's "Thank you for
playing" line (CODE:07B4) to the terminal.

## The setup screen

The window's build starts with the port's own setup screen (src/launch.c
on doskit's launcher.h, as its docs/LAUNCHER.md draws it: a menu with
one page per group; the headless build only with `-setup`): start
the game as it starts (intro, table menu) or one of the four tables at
once; skip the intro (the port's, off by default: the intro ends as a
key ends it); window or full screen; the game's options (balls, table
angle, scrolling, multiball maximum, tilt, resolution), stored in
ILLUSION.CFG as the game's own options screen stores them (the SVGA
mode byte cleared when the resolution changed, as CODE:32D5A does);
the sound (volume, bass, treble, deep bass, headphones: doskit's
audiofx on what the port plays; keypad + and - in play change the
volume, * mutes and unmutes it, and doskit's hud.h shows "VOLUME" with
ten steps or "MUTE" in a box at the top for two seconds, as pddnative
did; the mute is not kept in pinative.cfg); the keys (a key of the player's for each of
the game's, which work as well) and a controller's buttons. The
port's settings are in the data folder's pinative.cfg. None of it
changes how the game plays the options it reads.

## Build and run

    sh port/build.sh          # macOS, Linux (SDL2 for the window)
    port\build.bat            # Windows (MSVC)
    port/build/pinative -game game [-cue ".../data/game.ins"]
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
stays undefined. The setup screen and the dialogs about the game's
files name it in their title bar (doskit's LauncherApp); nothing else
reads it yet.
`PORT_UPDATE_URL` likewise, from the environment only: where a release
looks for newer ones (doskit/runtime/update.h); the workflow sets it to
the latest release's `latest.json`. Nothing reads it yet either: the
port has no setup screen of its own to ask on (RELEASE.md point 7).

The runtime is in the doskit submodule: after a clone or a pull,
`git submodule update --init` (the build fails with files not found
otherwise). Any `ILLUSION.CFG` the game's set-up wrote will do for
`-cfg`, whatever driver it names: the port always loads NOSOUND.SDR
(only dosrun's comparison runs need a file naming it). Without `-cfg`
the port takes `ILLUSION.CFG` in the game's folder, else the one in the
data folder (`%LOCALAPPDATA%\Pinball Illusions`, `~/.local/share/pinative`,
`~/Library/Application Support/Pinball Illusions`). With no file the
port does not run the game's sound set-up, as the original would: it
gives the game a header of its own naming NOSOUND.SDR and the options
all 0 (the port loads NOSOUND.SDR whatever is named, so a set-up
would change nothing), and makes the file at that place when the options are first
saved (the high scores). This is the port's behaviour, not the game's
(since 2026-10-01). `-opt s` still stops at SOUND_SETUP. A file with an
SVGA mode shows the table at 640x480 or 800x600 through doskit's VESA
modes (since 2026-10-01). At 800x600 the driver's frame measure took a
25.175 MHz dot clock for the mode's 40 MHz and the table ran at half
speed; fixed 2026-10-01 (checked headless: as many table frames as in
VGA 360 for the same pictures).

## Releases

`.github/workflows/build.yml` (doskit's template's since doskit e06b74d)
builds the packages doskit/docs/RELEASE.md prescribes; a pushed tag
`vX.Y` makes a release of them. `dist/README.txt` is the players' README
in each package, filled in for the chooser the port reaches so far.
`dist/uninstall-data.sh` (macOS, Linux) and `dist/uninstall-data.bat`
(Windows) go into the packages beside it: after asking they remove the
data folder's `game` and `cd` (the copied game files, CD image and
music) and keep pinative.cfg and ILLUSION.CFG, so a fresh copy from the
GOG release can be tried (since 2026-10-01).

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
- 2026-09-30, Linux: the same game on table 1 on to GAME_PHASE 7
  (dosrun `-break 12CA05`, CODE:2BAD5, t=160.340478, the ball lost under
  the ball save) and to GAME_PHASE 5 (`-break 12CAF6`, CODE:2BBC6,
  t=171.278049, the served ball lost too); tables 3 and 4 likewise to
  GAME_PHASE 5 (F1 and Enter as for GAME_PHASE 6; t=165.681288,
  165.212519): CODE differs only in FRAME_SPINS, TAIL and video memory
  equal, the heap blocks equal but the leftovers named above and the
  driver's block, which differs in its sample clock and in the channels'
  state (the module's position follows that clock; on tables 3 and 4 also
  the music state kept at a sound's start, C2D26..). The flippers were
  not pressed: flipper kinds 1, 2, 3, 5 and 6, the pushes of a turning
  flipper and event opcodes 1, 0Dh, 0Eh and 19h did not run (dosrun
  `-cover`), nor the pause, Esc in play or the tilt.
- 2026-09-30, Linux: GAME_PHASE 5 (dosrun with the argument
  `C:\ILLUSION.CFG`, keys as for GAME_PHASE 4), at the second ball's
  wait (`-break 12C10C#2`, CODE:2B1DC): table 1 with no bonus (t=173.94),
  table 1 with two players (`-key 150 3C`, the port's F2 at 7087;
  t=175.24), and tables 1, 3 and 4 with the player's bonus 12,345,
  multiplier 4 and 3 combos poked at CODE:2BBC6 (`-poke 12CAF6 10E7F0
  0000000045230100`, `10E7FA 0400` and the module's combo word, written
  by a scratch hook in the port, since removed; t=176.43, 170.70,
  170.24): CODE differs only in FRAME_SPINS, TAIL and video memory
  equal, the heap blocks equal but the leftovers of the GAME_PHASE 5
  comparison above (compared at the run's block addresses by a scratch
  script). Not run (dosrun `-cover`): the extra ball, game over, the
  tilt's bonus, a key ending FRAMES_WAIT, table 1's slot 41 with a held
  multiplier, a record's lamp at the next ball. Table 2 not reached (it
  stops in GAME_START). A whole game on table 1 with Enter every 500
  pictures now plays its three balls to `Stopped before CODE:2AA90`
  (not compared).
- 2026-09-30, Linux: GAME_PHASE 3 and 8 on table 1 (dosrun with the
  argument `C:\ILLUSION.CFG`, keys as for GAME_PHASE 4, the balls left,
  state+0D36h, poked to 1 at the first lost ball, `-poke 12CAF6 10E7A4
  0100`, so game over follows it; in the port the same pokes by a
  scratch hook, since removed), at the players' scores after it
  (`-break 12B4A3`, CODE:2A573, the first "PLAYER 1" page): no high
  score (t=176.79); the player's word +0 poked to 3 (`-poke 12CAF6
  10E7E8 0300`, 300,000,000, the third place) with A, B, Backspace, C,
  D typed (dosrun `-key 181 1E` .. `-key 185 20`, the port's at
  pictures 8812, 8847, 8882, 8917 and 8952, found by counting the name
  loop's passes: 66, 101, 136, 171, 206; t=190.48); and at the scores'
  end (`-break 12B5AE`, CODE:2A67E, t=204.74) with that score, the
  player count poked to 2 at CODE:2AA90 (`-poke 12B9C0 10E7DE 0200`)
  and A then Enter typed: CODE differs only in FRAME_SPINS (and
  FRAME_COUNT by one, the runner's timing, in the first two), TAIL and
  video memory equal, the heap blocks equal but the known leftovers
  and the driver's block (its sample clock, the module's position, and
  VSYNC_PHASE at CODE:0797 in the first). The extra ball (the player's
  byte +10h poked to 1 at the first lost ball, `-poke 12CAF6 10E7F8
  01`, Enter `-key 176 1C`, the port's at picture 8454): equal at
  GAME_PHASE 8's end (`-break 12CAD8`, CODE:2BBA8, t=178.34) and up to
  play's 840th frame (`-break 12C69E#840`); in the 841st the lost
  ball's BALLS_MOVE reads the slope map one line past its end, which
  in the original is pMAX's header of the next heap block (see
  docs/HANDOFF.md, "GAME_PHASE 3 and 8 in the port"), so the ball's
  record differs from there. All of GAME_OVER, OVER_WAIT, EXTRA_BALL
  and CODE:2A557's branch ran in these runs (dosrun `-cover`).
- 2026-09-30, Linux: pMAX's heap with its headers (src/pmax.c, see
  docs/HANDOFF.md, "pMAX's heap headers"): the heap's whole range
  (1473B0h to FF0000h) compared byte by byte (a scratch script) at the
  end of the two-player game over above (CODE:2A67E) and at the first
  lost ball on tables 3 and 4 (the runs of GAME_PHASE 4 above, CODE:2BBC6
  first hit): equal but the driver's block (its clock) and the free space
  at the top, where pMAX's own file buffers were (not modelled); the
  "known leftovers" in "Hidelights mask", "Temp Text area" and "Spooky"
  were stale headers and are equal now. The extra-ball game above is
  now equal at the second lost ball (CODE:2BBC6 second hit, t=180.07)
  and at the players' scores after its game over (CODE:2A573,
  t=185.58): CODE but FRAME_SPINS and FRAME_COUNT, video memory equal.
- 2026-09-30, Linux: the bumpers' and slingshots' kicks (BALL_BOUNCE,
  CODE:12F26 and 1302B) in a second game on table 1 after a one-ball
  first game (dosrun `-poke 12CAF6 10E7A4 0100 -key 195 3B -key 197.845
  1C -key 218.67 1C`, the port's F1 at picture 9461, Enter at 9561 and
  10695): equal at the game's phase-7 entries (`-break 12C91C#3`, #4,
  #5) and at its second ball's end (`-break 12CAF6#3`, t=233.10), after
  slingshot 1 and bumpers 3, 1 and 2: CODE but FRAME_SPINS, video
  memory equal, the heap but the driver's clock and the top's free
  space; both kicks and CODE:28E7B ran whole (dosrun `-cover`).
- 2026-10-01, Linux: OBJECT_HITS (CODE:2C7FC) with its three object
  types, in a blind game on table 1 with the flippers: the keys given
  by table frame to both (the port's DK_KEYS picture = the frame +
  5902; dosrun `-keyat 12A7F5#N KEY+` / `KEY-` at the Nth pass of
  CODE:298C5, N = the frame + 1): F1 at frame 1185, Enter 1299, then
  every 45 frames Left Shift and one frame later Right Shift, each held
  8 frames. Compared at CODE:298C5's passes 1700, 1791, 1794, 1850 and
  1911 (the port stopped there by a scratch stop; at 1911 it stops in
  ZONES_CHECK, zone type 4): CODE but FRAME_SPINS, video memory equal,
  the heap (1473B0h..FF0000h, compared by a scratch script) but the
  driver's block at 1473C0h, the top's free space and the last word of
  "table bin file relocation table"'s header. The ball hit an object
  of type 0 (index 1) in frame 1792; OBJECT_TYPE0 ran but for the
  branch of a light already set (dosrun `-cover`); DROP_HIT and
  OBJECT_TYPE2 did not run. Both shifts in the same frame do not
  compare: the run took the second key's interrupt about 6 ms later,
  after that frame's flipper reads (docs/HANDOFF.md, "The objects a ball
  hits in the port").
- 2026-10-01, Linux: a whole blind game on table 1 (doskit's new
  `-keysat 12A7F5 FILE`, the keys by CODE:298C5's pass as above): F1 at
  frame 1185, Enter 1299, every 45 frames Left Shift and a frame later
  Right Shift (8 frames each) and every 315 frames Enter, to game over
  at frame 5552 (holes taken and ejected, take handlers 6, 7, 0Bh, 10h,
  14h, display opcodes 6 and 9 ran; dosrun `-cover`). Compared at
  passes 1911, 1950, 2100, 2500, 3000, 4000, 5500, 7000 (the same
  keys without the Enters after the first: the second ball waits for
  its launch at 3000 and after) and 3000, 4300, 5490, 6000 (with them; 6000 in the attract
  mode after game over): CODE but FRAME_SPINS, video memory and the
  heap equal as before, but at 4300 FRAME_COUNT one behind and
  LIGHTS_DRAW_POS 0 for the run's 30h (a frame where the run's
  LIGHTS_DRAW was cut by the retrace; equal again at 5490).
- 2026-10-01, Linux: blind games as above with flips every 33 frames
  (Enter every 7th), table 1 (passes 1600, 2400, 3000, 5000, 7000,
  9000, 11000, 13000; game over between 5000 and 7000) and table 4
  (`-key 113 down -key 113.5 down -key 114 down` in the chooser, F1 at
  frame 1241; passes 2500, 4500, 6500, 8841, 9500, 11000, 13000,
  15000; game over between 11000 and 13000): CODE but FRAME_SPINS,
  video memory and the heap equal as before, but at a pass right after
  a key the port has taken and the run not yet (KEY_DOWN, LAST_KEY).
  Ran (dosrun `-cover`): a mode started and waiting (event opcodes 9,
  1Ch), a jingle and its end, take handlers 5, 6, 7, 0Bh, 10h, 11h,
  14h, 15h, 16h, DROP_HIT and OBJECT_TYPE2 (docs/HANDOFF.md, "The modes
  and the take handlers in the port", for what did not).
- 2026-10-01, Linux: table 2 (`-key 114 down`, F1 at frame 1161),
  the same blind keys, passes 2000, 4000, 6000, 8000: CODE but
  FRAME_SPINS and FRAME_COUNT (the run's 10 ahead at 8000: frames
  the run lost to its driver's wait for the retrace, the runner's timing,
  docs/HANDOFF.md "Table 2's FRAME_COUNT") equal, video memory and the heap equal as before.
- 2026-10-01, Linux: two balls against each other (BALLS_PAIR), table 2
  blind games as above with flips every 67 frames (Enter every 5th;
  passes 4753, 4754, 4756, 4800), 65 (every 9th; 5993, 6047, 6100) and
  89 (every 9th; 4890, 6026, 6105, 6110): contacts, turns (cases 0, 1,
  3, 4, 5, 6) and pushes ran before those passes; CODE but FRAME_SPINS
  and FRAME_COUNT (the run's 1 to 6 ahead), video memory and the heap
  equal as before (docs/HANDOFF.md, "The ball-ball physics in the port").
- 2026-10-01, Linux: table 2's music chooser (event opcode 14h, module
  object, TUNE_START/TUNE_UPDATE), forced by a poke at pass 2000 of the
  blind game with flips every 33 frames (dosrun `-poke`, a scratch poke
  in the port): passes 2150, 2192, 2210, 2230, 2248, 2400, 3000, CODE
  but the runner's timing and the keys, video memory and the heap equal
  as before (docs/HANDOFF.md, "Table 2's music chooser in the port");
  only tune 0 chosen.
- 2026-10-01, Linux: the table's end and the chooser again (table 1:
  Esc and Y in the attract mode, the port's pictures 7087 and 7157,
  then Enter at 7406 and 7580, dosrun's t=150, 152, 162, 165): at
  CODE:7182, the second chooser's CODE:4FF9, its FRAME_WAIT calls 460,
  550, 650, the second CODE:A323 and passes 1300, 1500, 2000, 3000 of
  the second visit: CODE but the runner's timing, video memory and the
  heap equal as before (docs/HANDOFF.md, "The table's end and the
  chooser again").
- 2026-10-01, Linux: the high scores' file (INT 94h AH=6,
  pmax_cfg_write): table 1 left by Esc and Y with HISCORES' first byte
  changed at TABLE_END (dosrun `-poke`, a scratch line in the port): the
  file written equal byte for byte to dosrun's (docs/HANDOFF.md, "The
  high scores' file"). Not run: a high score reached by play.
- 2026-10-01, Linux: the pause (PLAY_KEYS's CODE:2B51C, PAUSE_QUIT),
  table 1, keys by table frame as above: F1 1185, Enter 1299, P 1400,
  Space 1750 (CODE:298C5's passes 1405, 1700, 1760, 1900) and P 1400,
  Esc 1450, Space 1500, Esc 1550, Y 1600 (passes 1460, 1505, 1555; the
  chooser after Y: the run after 600 FRAME_WAIT calls against the port
  after 601): CODE but the runner's timing, video memory and the heap equal
  as before (docs/HANDOFF.md, "The pause in the port"). Not run: a
  pause with music playing.
- 2026-10-01, Linux: BCD_COUNTERS_STEP (CODE:2EAC9), table 1, a
  subtracting counter started by a poke at CODE:298C5's pass 1400 in
  both (docs/HANDOFF.md, "The BCD counters in the port"): passes 1401
  to 3100 and, with a start below the end, 1480 to 1500: CODE but
  FRAME_SPINS, video memory, the counter and the heap equal as before;
  the same for an adding counter of table 3 (passes 1501 to 1720, its
  end at 1700).
- 2026-10-01, Linux: the take handlers no blind game ran (1, 2, 8,
  0Ah, 0Eh, 0Fh, 12h, 13h, 17h, 18h, 1Ah, 1Bh), each forced by a poke
  of the skill-shot streams in both, tables 1, 2 and 3: at two passes
  each CODE but FRAME_SPINS, video memory and the heap equal as before
  (docs/HANDOFF.md, "The take handlers forced in a run").
- 2026-09-30, Windows (MSVC, build.bat) by the user: built and the
  intro seen playing to its end in the window, with the user's
  Sound Blaster ILLUSION.CFG. The build's warnings not looked at.
- 2026-10-01, Windows (MSVC) by the user: built without errors (no
  run reported).
- 2026-10-01, CD audio: the port answers MSCDEX from the GOG release's
  cue sheet (src/cd.c over doskit's cdaudio.h; `-cue FILE`, else the
  `game.ins` or `game.inst` beside the image gog_find finds, with or
  without `-game` since the user's Windows run with `-game` had no table
  music; `-cue none` or none found: one data track as before, which the
  comparisons before 2026-10-01 used): the table of contents (IOCTL input
  0Ah, 0Bh), play (84h), stop (85h: a play paused, else forgotten),
  resume (88h), the channels and volumes (IOCTL output 3); the tracks
  mixed into the port's audio over the driver's music. Checked on Linux
  against dosrun with `-cue data/game.ins` and NOSOUND.SDR: at the
  chooser's 100th CODE:38FD (DK_FRAMES 5500) CODE differs only in
  FRAME_SPINS (the track table CD_READ_TOC fills equal); at table 1's
  700th frame (Enter at 112 and 115, `-break 12A7F5#700`, DK_KEYS 5637
  and 5815, DK_FRAMES 6602) only in the bytes one frame apart as before
  (FRAME_COUNT, TRACK_LEFT, ...). A scratch print in cd.c: table 1
  plays frames 24470 (track 2's start) for 13717. The real tracks
  decoded (a scratch program: tracks 2, 14, 26, 38, 50, RMS 4100 to
  8200). Not listened to; the window builds not run.
- 2026-10-01, sound: the port plays what NOSOUND.SDR mixes. The
  driver mixes the module into its DMA buffer (DMA_SEL, mono words at
  MIX_RATE, 44100) as it does under DOS, where no card reads it; the
  port (src/nosound.c, `out_sample`, its own, not the driver's) takes
  each word as the timer's SAMPLE_POS passes it, as a card's DMA would,
  and hands it through a FIFO (at most 4096 samples kept, the oldest
  dropped) to the platform's audio, both stereo channels alike. Checked
  on Linux, headless, by a scratch build writing the words to a file:
  the intro's first 2500 pictures give 41.7 s of samples, rising from
  peaks of 912 to 31040, no jump over 30000 between two samples (no
  16-bit wrap), about 1600 zero crossings a second. Not listened to;
  the window builds (SDL, Win32) not run; the pause (command 2, the
  timer stopped) gives silence by the FIFO running dry, not checked.
- 2026-10-01, Linux, headless: the chooser's caption, which flickered
  (every other picture without it, a white line at the right; the user
  saw it on Windows), shown in every picture since the port uses
  doskit's `frame_set_scanout_end` (main.c; docs/HANDOFF.md,
  "CHOOSER_WAIT's loop"): pictures 5600..5603 looked at, the intro's
  1500 and table 1's 6600 as before. The window build not tried.
- 2026-10-01, Windows by the user: with no ILLUSION.CFG the port
  stopped at SOUND_SETUP. Since then (Linux, headless, `-cfg` naming a
  missing file): it runs past SETUP_ARGS into the attract mode (stopped
  by `timeout` after 120 s), no file written; `-opt s` stops at
  SOUND_SETUP. The file made by a first save not tried; not run on
  Windows since.
- 2026-10-01, Linux, headless: the tilt (GAME_PHASE 9, CODE:2B716;
  docs/HANDOFF.md, "GAME_PHASE 9 in the port"), which stopped the port
  before: three Space presses 6 pictures apart in play on table 1 tilt,
  "TILT" shown, the ball lost, the next ball served. Not compared with
  the original's memory; the window builds not tried.
- 2026-10-01, Linux, headless: the SVGA resolutions (OPT_RESOLUTION 1
  and 2; SVGA_CHECK, MODE_SVGA640, MODE_SVGA800 and doskit's VESA modes;
  docs/HANDOFF.md, "The SVGA modes in the port"), which stopped the
  port before: the table in 640x480 and 800x600 pictures, the 336
  pixels in the middle; on 800x600 a game of three balls. Not compared
  with the original's memory; the window builds not tried.
- 2026-10-01, Linux, headless: the CD copied into the data folder's
  `cd` at the first start (doskit's cd_copy_disc: the GOG release's
  game.ins, game.gog and MUSIC, 134 MB, from ~/GOG Games/Pinball
  Illusions/data, with `DK_DATA_DIR` a scratch folder) and its cue
  sheet read from there; the run went on to the chooser. Whether the
  music plays from the copy not listened to; Windows and macOS not
  tried.
- 2026-10-01: the window builds ask before copying from the GOG release
  (main.c's offer: Y or Enter copies, N or Esc not; asked once for the
  game's files and the CD together; not asked with -gog or in the
  headless build), as doskit/docs/RELEASE.md and port/dist/README.txt
  say. Built on Linux without warnings, the headless first start (no
  question) copied `game` and `cd` as before; the question itself not
  seen, no window tried.
- 2026-10-03, Linux: doskit to 69d03a2, the setup screen rebuilt as the
  kit's menu with one page per group (src/launch.c: the Play actions,
  Skip the intro and Display in the menu, Game/Sound/Keys/Controller
  pages; no Quit of its own); main.c kept as the joint offer of
  2026-10-01 (the game's files and the CD's image and music together).
  Headless with `-setup` (screenshots): the menu and all four pages
  again after the merge, Law 'n Justice started at once from the menu
  (the table on screen); the joint offer declined (the program ends).
  The copy taken with its bar, the "not found" and "could not be
  copied" screens not tried (the kit's dialogtest covers them).
  Windows 2026-10-03 (the user's test, not the agent's): build.bat
  builds; the first start's joint dialog copies the game and the CD,
  the setup screen's menu and pages work, a table loads and a ball
  plays. macOS not tried.
=======
  seen, no window tried.
- 2026-10-01: that question replaced by doskit's dialog about the
  game's files (launcher.h, the one every port of the kit shows): the
  setup screen's backdrop ("Pinball Illusions Setup", the port's name
  and version), a window "The game's files" with the release found and
  the data folder, "Copy the files" or "Quit" (Up/Down, Enter; Esc
  quits), a bar with the file's name while copying, and the same
  window when nothing was found or the copy failed. Asked once for the
  game's files and the CD's image and music together; for the CD alone
  ("Copy the files" or "Not now", which plays from the release's cue
  sheet) when the game's files are there and the data folder's `cd` is
  not. Not asked with -gog (the bar is shown); the headless build
  copies as before, without the dialog. Closing the window while
  copying ends the program. Built on Windows with MSVC (build.bat),
  check.py all ok; the dialog's screens seen as screenshots of
  doskit's tests/launcher/dialogtest.c only: pinative's window, its
  first start and the copy from a GOG release not run with this
  change, Linux and macOS not built.
- 2026-10-01: the setup screen's title bar as the dialog's: "Pinball
  Illusions Setup" at the left, "pinative" and the release's version
  at the right, where the game's name stood in the middle (doskit's
  launcher_run takes the names, main.c's pi_app, as the dialog does).
  Built on Windows with MSVC, check.py all ok; the bar seen in a
  screenshot of doskit's tests/launcher/launchtest.c, pinative's own
  screen not looked at.
- 2026-10-01: the title bar's left side is the game's name alone,
  "Pinball Illusions", without "Setup", on the setup screen and in the
  dialog about the game's files (doskit's launcher.c). Seen in a
  screenshot of launchtest again; pinative not built again for it (the
  change is in the kit's file only).
- 2026-10-01, Linux, headless with `-setup`: the setup screen's pages
  (screenshots), Babewatch started at once from it (in SVGA 640x480,
  the option saved), the intro skipped (the table menu at picture 350),
  pinative.cfg written and read back. The volume box 2026-10-01,
  headless, shown by a temporary call (the null platform gives no
  sound keys): at the top in VGA 360 and twice the size at 800x600.
  Not tried: the window builds,
  the keys and a controller in play, the volume keys, the sound
  settings heard.
- Not built on macOS since.

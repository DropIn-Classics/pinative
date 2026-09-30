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
the captions compiled into code as the original makes them), up to
CHOOSER (CODE:4FF9), where the port stops (`Stopped before CHOOSER`).
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

The runtime is in the doskit submodule: after a clone or a pull,
`git submodule update --init` (the build fails with files not found
otherwise). Any `ILLUSION.CFG` the game's set-up wrote will do for
`-cfg`, whatever driver it names: the port always loads NOSOUND.SDR
(only dosrun's comparison runs need a file naming it). Without `-cfg`
the port stops at SOUND_SETUP; a file with an SVGA mode stops it at
SVGA_CHECK.

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
- 2026-09-30, Windows (MSVC, build.bat) by the user: built and the
  intro seen playing to its end in the window, with the user's
  Sound Blaster ILLUSION.CFG. The build's warnings not looked at.
- Not built on macOS since.

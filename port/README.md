# port: Pinball Illusions in C

A native compatibility implementation requiring an installed copy of the
original game, on doskit's runtime (doskit/runtime). The game's data is
read from the player's copy at run time; none of it is in this
repository.

## State

Started 2026-09-29 from doskit's template: finds or unpacks the game's
files and shows a text screen. Since 2026-09-29 it unpacks
`ILLUSION.386` from the player's `ILLUSION.EXE` (src/archive.c, the
archive as tools/illfiles.py reads it) and loads it into doskit's
`pmem.h` memory as pMAX does (src/image.c: linear 100F30h, selector 1Ch;
the SHA-256 of src/gen/names.h checked). Translated: ENTRY and
SETUP_ARGS (src/entry.c, src/setup.c; the configuration file and
SETSOUND.DAT through src/pmax.c, in place of pMAX's services) and
SVGA_CHECK for VGA, VGA_INIT (the "Loading" picture) and HISCORE_INIT
(src/video.c), up to CODE:757D, where the port stops (`Stopped before
L757D`); `-opt o`,
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
- Not built with MSVC (build.bat changed alike) or on macOS since.

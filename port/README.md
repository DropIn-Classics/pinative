# port: Pinball Illusions in C

A native compatibility implementation requiring an installed copy of the
original game, on doskit's runtime (doskit/runtime). The game's data is
read from the player's copy at run time; none of it is in this
repository.

## State

Started 2026-09-29 from doskit's template: finds or unpacks the game's
files and shows a text screen. Nothing of the game is translated yet.
The game folder is recognised by `ILLUSION.EXE` (also what must come out
of the GOG image when it is unpacked); built with build.sh on macOS
2026-09-29, the image unpacking not tried.

## Build and run

    sh port/build.sh          # macOS, Linux (SDL2 for the window)
    port\build.bat            # Windows (MSVC)
    port/build/pinative -game game

## Checked

(what was compared with the original, where and how)

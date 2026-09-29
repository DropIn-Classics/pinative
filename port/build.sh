#!/bin/sh
# Builds build/pinative (the window, on SDL2) and build/pinative-headless
# (for tests and scripted runs, doskit/runtime/plat_null.c) with cc
# (clang or gcc), on macOS and Linux; build.bat is the same for Windows.
# Without SDL2 (doskit/runtime/sdl2-flags.sh) only the headless one.
set -e
cd "$(dirname "$0")"
CC=${CC:-cc}
RT=../doskit/runtime
CFLAGS="-O2 -Wall -Wextra -I$RT -Isrc"
GAME="src/main.c src/archive.c src/image.c src/pmax.c src/entry.c src/setup.c src/video.c"
RUNTIME="$RT/sys.c $RT/cdimage.c $RT/textmode.c $RT/pad.c $RT/sha256.c $RT/pmem.c $RT/vga.c $RT/frame.c $RT/modplay.c $RT/audiofx.c $RT/fli.c $RT/shot.c"

mkdir -p build
$CC $CFLAGS -o build/pinative-headless $GAME $RUNTIME $RT/plat_null.c -lm
if ! sdl=$(sh $RT/sdl2-flags.sh); then
    echo "SDL2 not found: built build/pinative-headless only." >&2
    exit 0
fi
# $sdl unquoted: it is a list of options
$CC $CFLAGS -o build/pinative $GAME $RUNTIME $RT/plat_sdl.c $sdl -lm

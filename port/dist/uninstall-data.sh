#!/bin/sh
# uninstall-data.sh - removes the game's files the port copied into its
# data folder (the folders "game" and "cd": the game's files, the CD's
# image and music), so that the next start copies them again from the
# GOG release.  The settings (pinative.cfg) and the game's options and
# high scores (ILLUSION.CFG) stay.  macOS and Linux; DK_DATA_DIR names
# another data folder, as for the program.
if [ -n "$DK_DATA_DIR" ]; then
    dir=$DK_DATA_DIR
elif [ "$(uname)" = Darwin ]; then
    dir="$HOME/Library/Application Support/Pinball Illusions"
else
    dir="${XDG_DATA_HOME:-$HOME/.local/share}/pinative"
fi
if [ ! -d "$dir/game" ] && [ ! -d "$dir/cd" ]; then
    echo "No game files in $dir: nothing to remove."
    exit 0
fi
echo "This removes the game's files the port copied into"
echo "    $dir"
echo "(the folders game and cd). Settings and high scores stay."
printf "Remove them? [y/N] "
read answer
case $answer in
    y|Y|yes|Yes) ;;
    *) echo "Nothing removed."; exit 0 ;;
esac
rm -rf "$dir/game" "$dir/cd" && echo "Removed. The next start copies them again."

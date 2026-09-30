pinative
========

A native compatibility implementation requiring an installed copy of
Pinball Illusions: this package contains only the program, our own
code. The game's data comes from the player's Pinball Illusions of
GOG.com and is read from it each time the program runs; without it
nothing can be played.

Not playable yet: the program shows the intro and the table chooser
(with its Info pages) and stops when a table is chosen.

Starting
--------

Start pinative (pinative.exe on Windows, pinative.app on a Mac). The
first time it looks for your GOG release: where GOG installed it (on
Windows found through the registry too), the GOG app in /Applications
or ~/Applications on a Mac, beside the program. It offers to copy the
game's files from it into a folder "game" beside the program (on a Mac
into ~/Library/Application Support/Pinball Illusions; on Linux, where
the program's folder cannot be written, ~/.local/share/pinative). If it
is not found, copy game.gog beside the program (on a Mac into
~/Library/Application Support/Pinball Illusions), or name it, or the
folder of the game's files (the one holding ILLUSION.EXE):

    pinative -gog /path/to/game.gog

Windows: pinative.exe needs nothing else. It is not signed, so Windows
may say it protected your PC: click "More info", then "Run anyway".

macOS (10.13 or newer, Intel and Apple silicon): pinative.app needs
nothing else; move it to Applications if you like. It is not signed by
Apple, so the first start is refused ("cannot be verified"): close that
message, open System Settings > Privacy & Security, click "Open Anyway"
at the bottom and confirm. After that it starts with a double click. On
macOS 14 and older a right click on the app, "Open" and "Open" again
does the same. Or, in the Terminal, in the folder of the app:

    xattr -cr pinative.app

Linux and the Steam Deck: keep libSDL2-2.0.so.0 beside pinative (the
package brings SDL2 along; nothing needs to be installed). On the Deck
the game starts full screen; in Game Mode add pinative as a non-Steam
game.

Keys
----

Alt+Enter: full screen on and off. Print Screen: a screenshot.

Intro: Space or Esc leaves it.
Table chooser: the arrow keys choose, Enter or Space takes the choice,
F1 to F4 take a table at once; Esc leaves an Info page, else ends the
program.

Licences
--------

MOD playback: micromod, by Martin Cameron (LICENCE-micromod.txt).
Linux and macOS: SDL2, by Sam Lantinga and others (LICENCE-SDL2.txt).

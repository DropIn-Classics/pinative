@echo off
rem uninstall-data.bat - removes the game's files the port copied into its
rem data folder (the folders "game" and "cd": the game's files, the CD's
rem image and music), so that the next start copies them again from the
rem GOG release.  The settings (pinative.cfg) and the game's options and
rem high scores (ILLUSION.CFG) stay.  DK_DATA_DIR names another data
rem folder, as for the program.
setlocal
if defined DK_DATA_DIR (set "dir=%DK_DATA_DIR%") else (set "dir=%LOCALAPPDATA%\Pinball Illusions")
if not exist "%dir%\game\" if not exist "%dir%\cd\" (
    echo No game files in %dir%: nothing to remove.
    pause
    exit /b 0
)
echo This removes the game's files the port copied into
echo     %dir%
echo (the folders game and cd). Settings and high scores stay.
set "answer="
set /p "answer=Remove them? [y/N] "
if /i not "%answer%"=="y" if /i not "%answer%"=="yes" (
    echo Nothing removed.
    pause
    exit /b 0
)
if exist "%dir%\game\" rmdir /s /q "%dir%\game"
if exist "%dir%\cd\" rmdir /s /q "%dir%\cd"
echo Removed. The next start copies them again.
pause

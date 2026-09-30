@echo off
rem Builds build\pinative.exe (the window) and build\pinative-headless.exe
rem (for tests and scripted runs) with MSVC.  If cl.exe is not on PATH,
rem vcvars64.bat is looked for: the VS2019 Build Tools first, then whatever
rem vswhere finds.  The C runtime is linked in (/MT): no redistributable.
setlocal
cd /d "%~dp0"

where cl >nul 2>&1
if %errorlevel% neq 0 (
  if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
  ) else (
    for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do (
      if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    )
  )
)
where cl >nul 2>&1
if %errorlevel% neq 0 (
  echo No MSVC compiler found. Install the Visual Studio 2019 Build Tools with the C++ toolchain.
  exit /b 1
)

set RT=..\doskit\runtime
if not exist build\obj\headless mkdir build\obj\headless
set CFLAGS=/nologo /W4 /O2 /MT /D_CRT_SECURE_NO_WARNINGS /I%RT% /Isrc
rem the release's version: %PORT_VERSION%, else the tag of the commit built;
rem none, and PORT_VERSION stays undefined
if not defined PORT_VERSION for /f "usebackq delims=" %%v in (`git describe --tags --exact-match 2^>nul`) do set PORT_VERSION=%%v
if defined PORT_VERSION set CFLAGS=%CFLAGS% /DPORT_VERSION=\"%PORT_VERSION%\"
set GAME=src\main.c src\archive.c src\image.c src\pmax.c src\entry.c src\setup.c src\video.c src\cd.c src\sound.c src\intro.c src\hostcb.c src\nosound.c src\nsplay.c src\chooser.c src\table.c
set RUNTIME=%RT%\sys.c %RT%\cdimage.c %RT%\textmode.c %RT%\pad.c %RT%\sha256.c %RT%\pmem.c %RT%\vga.c %RT%\frame.c %RT%\modplay.c %RT%\audiofx.c %RT%\fli.c %RT%\shot.c

cl %CFLAGS% /Fobuild\obj\ /Fe:build\pinative.exe %GAME% %RUNTIME% %RT%\plat_win32.c user32.lib gdi32.lib winmm.lib advapi32.lib shell32.lib /link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
if errorlevel 1 exit /b 1
cl %CFLAGS% /Fobuild\obj\headless\ /Fe:build\pinative-headless.exe %GAME% %RUNTIME% %RT%\plat_null.c advapi32.lib shell32.lib
if errorlevel 1 exit /b 1

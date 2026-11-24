#!/bin/sh
# Build the Windows version on Linux with MinGW-w64 and pack it as dist/conflict-windows.zip:
#   conflict-port/conflict.exe, SDL3.dll, HOW-TO-PLAY.txt
# Needs the MinGW-w64 C++ compiler and SDL3 for MinGW (Fedora: mingw64-gcc-c++ mingw64-SDL3),
# CMake and zip. The original game files are NOT included.
set -e
cd "$(dirname "$0")/.."

TOOLCHAIN=${MINGW_TOOLCHAIN:-/usr/share/mingw/toolchain-mingw64.cmake}
cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build-win --target conflict -j

rm -rf dist/conflict-port dist/conflict-windows.zip
mkdir -p dist/conflict-port
cp build-win/conflict.exe build-win/SDL3.dll dist/conflict-port/
x86_64-w64-mingw32-strip dist/conflict-port/conflict.exe 2>/dev/null || true

# Windows line endings so Notepad shows it properly everywhere.
sed 's/$/\r/' > dist/conflict-port/HOW-TO-PLAY.txt <<'EOF'
CONFLICT: Middle East Political Simulator - Windows version of the port
=======================================================================

This is a new version of the 1990 MS-DOS game by David J. Eastman. It needs the
original game files, which are not included. They are free to download:

1. Download the original game from the Internet Archive:
   https://archive.org/download/conflict_201509/conflict.zip

2. Unpack conflict.zip (right-click it, "Extract All..."). It contains a folder
   called "conflict". Move that folder into this folder, next to conflict.exe:

     conflict-port\
       conflict.exe
       SDL3.dll
       HOW-TO-PLAY.txt
       conflict\
         Conflict.exe
         ISRAEL.MP
         ... (the other game files)

   (If the unpacked files also contain a "__MACOSX" folder, ignore it.)

3. Double-click conflict.exe to play. First a launcher asks which maps to
   use: the original 1990 maps (the default) or new redrawn ones. Arrow keys
   to choose, Enter to start.

Keys: arrows / Home / End to choose, Enter to confirm, Shift-B briefing,
Shift-S save (at the newspaper), Shift-E quit, F11 full screen.

The game's manual is on the same Internet Archive page:
https://archive.org/details/conflict_201509 (ConflictManual.pdf)

Windows may warn that the program is from an unknown publisher ("Windows
protected your PC"). Click "More info", then "Run anyway".

To play with the original 1990 bugs left in, start it from a command prompt as
  conflict.exe --original-bugs

Source code, details and the list of fixed bugs:
https://github.com/s-papanastasiou/conflict-port

SDL3.dll is the SDL library (https://libsdl.org), zlib licence.
EOF

(cd dist && zip -qr conflict-windows.zip conflict-port)
echo "dist/conflict-windows.zip"

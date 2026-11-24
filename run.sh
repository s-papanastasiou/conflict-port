#!/bin/sh
# Start CONFLICT. Builds the port first if needed, then runs it with the original game files in
# ./conflict. Extra options are passed on, e.g. ./run.sh --original-bugs
# F11 switches between window and fullscreen.
cd "$(dirname "$0")" || exit 1

# Show a message in a dialog when started from a file manager, otherwise on the terminal.
say() {
    if [ ! -t 1 ] && command -v zenity >/dev/null 2>&1; then
        zenity --error --title="CONFLICT" --text="$1" 2>/dev/null
    elif [ ! -t 1 ] && command -v kdialog >/dev/null 2>&1; then
        kdialog --title "CONFLICT" --error "$1" 2>/dev/null
    elif [ ! -t 1 ] && command -v notify-send >/dev/null 2>&1; then
        notify-send "CONFLICT" "$1"
    fi
    echo "$1" >&2
}

GAME_DIR=conflict
if ! ls "$GAME_DIR" 2>/dev/null | grep -qi '^conflict\.exe$'; then
    say "The original game files were not found.

They go in this folder:
$(pwd)/$GAME_DIR

To get them from the Internet Archive, run these two commands in $(pwd):
  curl -L -o conflict.zip https://archive.org/download/conflict_201509/conflict.zip
  unzip conflict.zip -x '__MACOSX/*'

Or download https://archive.org/download/conflict_201509/conflict.zip in a browser,
unpack it and move the 'conflict' folder it contains into $(pwd).
Then start ./run.sh again. (Details: README.md, 'How to play'.)"
    exit 1
fi

# Build (or rebuild) when the binary is missing or older than the sources.
if [ ! -x build/conflict ] || [ -n "$(find src CMakeLists.txt -newer build/conflict 2>/dev/null | head -n 1)" ]; then
    echo "Building CONFLICT..."
    if ! { cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null && cmake --build build --target conflict -j; }; then
        say "Building the game failed. It needs CMake, a C++20 compiler and SDL3.
Run ./run.sh in a terminal to see the details."
        exit 1
    fi
fi

exec ./build/conflict --game "$GAME_DIR" "$@"

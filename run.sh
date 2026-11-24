#!/bin/sh
# Start CONFLICT. Builds the port first if needed, then runs it with the original game files in
# ./conflict. Extra options are passed on, e.g. ./run.sh --original-bugs
# F11 switches between window and fullscreen. If the game files are missing, the game itself says
# where to get them (in a message box).
cd "$(dirname "$0")" || exit 1

# Only for a failed build, when there is no game yet to show a message box: a dialog if a dialog
# tool happens to be installed and there is no terminal, and always on the terminal.
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

# Build (or rebuild) when the binary is missing or older than the sources.
if [ ! -x build/conflict ] || [ -n "$(find src CMakeLists.txt -newer build/conflict 2>/dev/null | head -n 1)" ]; then
    echo "Building CONFLICT..."
    if ! { cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null && cmake --build build --target conflict -j; }; then
        say "Building the game failed. It needs CMake, a C++20 compiler and SDL3.
Run ./run.sh in a terminal to see the details."
        exit 1
    fi
fi

exec ./build/conflict --game conflict "$@"

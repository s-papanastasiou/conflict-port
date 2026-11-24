#!/bin/sh
# Build a Linux version that runs on most distributions and pack it as
# dist/conflict-linux-x86_64.tar.gz:
#   conflict-port/conflict-port (the program), libSDL3.so.0, HOW-TO-PLAY.txt
# (not named "conflict": that is the folder the original game files go in)
# It is built in an Ubuntu 22.04 container (glibc 2.35), so it runs on Ubuntu 22.04+, Debian 12+,
# Linux Mint 21+, Fedora 36+, Arch and others of that age or newer. SDL3 is built from source and
# shipped next to the program; the C++ runtime is linked in. Needs podman (or docker) and network
# access for the first run. The original game files are NOT included.
set -e
cd "$(dirname "$0")/.."

SDL_VERSION=3.4.20
CMAKE_VERSION=3.31.6
ENGINE=${CONTAINER_ENGINE:-$(command -v podman || command -v docker)}
[ -n "$ENGINE" ] || { echo "needs podman or docker" >&2; exit 1; }

rm -rf dist/conflict-port-linux dist/conflict-linux-x86_64.tar.gz
mkdir -p dist/conflict-port-linux

"$ENGINE" run --rm -v "$PWD:/src:ro,Z" -v "$PWD/dist/conflict-port-linux:/out:Z" ubuntu:22.04 bash -euc "
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq --no-install-recommends g++-12 ninja-build pkg-config curl ca-certificates \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev \
  libxkbcommon-dev libwayland-dev wayland-protocols libegl1-mesa-dev libgl1-mesa-dev libdrm-dev \
  libgbm-dev libdecor-0-dev libudev-dev libdbus-1-dev libpulse-dev libasound2-dev >/dev/null
export CC=gcc-12 CXX=g++-12
curl -sSL https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION-linux-x86_64.tar.gz | tar xz -C /opt
export PATH=/opt/cmake-$CMAKE_VERSION-linux-x86_64/bin:\$PATH
curl -sSL https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL3-$SDL_VERSION.tar.gz | tar xz -C /tmp
cmake -S /tmp/SDL3-$SDL_VERSION -B /tmp/sdl-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/sdl3 -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF -DSDL_STATIC=OFF >/dev/null
cmake --build /tmp/sdl-build >/dev/null
cmake --install /tmp/sdl-build >/dev/null
cmake -S /src -B /tmp/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/sdl3 \
  -DCONFLICT_PORTABLE=ON >/dev/null
cmake --build /tmp/build --target conflict maps_tests
/tmp/build/maps_tests
cp /tmp/build/conflict /out/conflict-port
cp -L /opt/sdl3/lib/libSDL3.so.0 /out/
strip /out/conflict-port /out/libSDL3.so.0
"

cat > dist/conflict-port-linux/HOW-TO-PLAY.txt <<'EOF'
CONFLICT: Middle East Political Simulator - Linux version of the port
=====================================================================

This is a new version of the 1990 MS-DOS game by David J. Eastman. It needs the
original game files, which are not included. They are free to download.

In a terminal, in this folder (conflict-port), run:

  curl -L -o conflict.zip https://archive.org/download/conflict_201509/conflict.zip
  unzip conflict.zip -x '__MACOSX/*'
  ./conflict-port

(Without the terminal: download https://archive.org/download/conflict_201509/conflict.zip,
unpack it and move the "conflict" folder it contains into this folder. Then
double-click "conflict-port".)

The folders must end up like this:

  conflict-port/
    conflict-port       (the program)
    libSDL3.so.0
    HOW-TO-PLAY.txt
    conflict/
      Conflict.exe
      ISRAEL.MP
      ... (the other game files)

First a launcher asks which maps to use: the original 1990 maps (the default)
or new redrawn ones. Arrow keys to choose, Enter to start.

Keys: arrows / Home / End to choose, Enter to confirm, Shift-B briefing,
Shift-S save (at the newspaper), Shift-E quit, F11 full screen.

The game's manual is on the same Internet Archive page:
https://archive.org/details/conflict_201509 (ConflictManual.pdf)

To play with the original 1990 bugs left in: ./conflict-port --original-bugs

Runs on Linux distributions from about 2022 on (glibc 2.35 or newer, x86-64).
Source code, details and the list of fixed bugs:
https://github.com/s-papanastasiou/conflict-port

libSDL3.so.0 is the SDL library (https://libsdl.org), zlib licence.
EOF

mv dist/conflict-port-linux dist/conflict-port-linux.tmp
mkdir -p dist/linux && rm -rf dist/linux/conflict-port && mv dist/conflict-port-linux.tmp dist/linux/conflict-port
tar -C dist/linux -czf dist/conflict-linux-x86_64.tar.gz conflict-port
echo "dist/conflict-linux-x86_64.tar.gz"

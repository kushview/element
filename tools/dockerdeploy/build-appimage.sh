#!/bin/sh
# Builds a Linux AppImage

set -e

export CFLAGS="-DEL_APPIMAGE=1"
export CXXFLAGS="-DEL_APPIMAGE=1"
./waf configure --prefix=/usr \
    --depends="/depends/x86_64-pc-linux-gnu" \
    --depends-allow-system
./waf build -j4
./waf docs

export LD_LIBRARY_PATH="/depends/x86_64-pc-linux-gnu/lib"
sh tools/appimage.sh
cp -fv build/*.AppImage /dist/

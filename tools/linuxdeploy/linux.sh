#!/bin/sh
set -e
here=`pwd`

sh tools/buildall-linux.sh --depends="/depends/x86_64-pc-linux-gnu" --depends-allow-system
mkdir -p dist && cp build/*.AppImage dist/
./waf dist && cp -f *.tar.* dist/
./waf distclean

./waf configure --depends="/depends/x86_64-w64-mingw32"
./waf build
mkdir -p dist/element-win64-`./waf version`
cp build/bin/* dist/element-win64-`./waf version`

./waf distclean
rm -rf build/*
rm -f .lock*

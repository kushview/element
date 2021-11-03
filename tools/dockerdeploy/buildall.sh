#!/bin/sh
set -e
here=`pwd`

deep_clean()
{
    ./waf distclean
    rm -rf build/*
    rm -f .lock*
    find . -name "*.pyc" -delete
}

deep_clean

sh tools/buildall-linux.sh \
    --depends="/depends/x86_64-pc-linux-gnu" \
    --depends-allow-system
cp build/*.AppImage /dist/
./waf dist && cp -f *.tar.* /dist/
deep_clean

./waf configure --depends="/depends/x86_64-w64-mingw32" 
./waf build -j2
win64dir="dist/element-win64-`./waf version`"
mkdir -p "$win64dir"
cp build/bin/* "$win64dir"
deep_clean

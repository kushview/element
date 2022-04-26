#!/bin/sh
# Builds a Linux AppImage

set -e

export CFLAGS="-DEL_APPIMAGE=1"
export CXXFLAGS="-DEL_APPIMAGE=1"
./waf configure --prefix=/usr \
    --depends="/depends/x86_64-pc-linux-gnu" \
    --depends-allow-system \
    --with-vstsdk24="/SDKs/vstsdk2.4" \
    CC="clang" CXX="clang++" \
    ${WAF_CONFIGURE_OPTIONS}
./waf build ${WAF_BUILD_OPTIONS}
./waf docs

export LD_LIBRARY_PATH="/depends/x86_64-pc-linux-gnu/lib"
sh tools/lindeploy/appimage.sh
cp -fv build/*.AppImage /dist/

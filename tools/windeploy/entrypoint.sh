#!/bin/sh
# Builds Element standalone for windows using mingw32 gcc

set -e
here=`pwd`

./waf configure --depends="/depends/x86_64-w64-mingw32" \
    --with-vstsdk24="/SDKs/vstsdk2.4" \
    --with-vst3sdk="/SDKs/vst3sdk" \
    --with-asiosdk="/SDKs/asiosdk" \
    --prefix="/" \
    --scriptsdir="/scripts" \
    --luadir="/lua"
./waf build ${WAF_BUILD_OPTIONS}

pkgname="element-w64-$(python waf version --revision)"
win64dir="/dist/${pkgname}"
./waf copydlls install --destdir="$win64dir"
cp -f build/lib/*.dll "$win64dir/lib"
cp -f tools/element.bat "$win64dir"

find "$win64dir" -name \*.exe -exec x86_64-w64-mingw32-strip {} \;
find "$win64dir" -name \*.dll -exec x86_64-w64-mingw32-strip {} \;

cd /dist && zip -rvT "${pkgname}.zip" "${pkgname}"

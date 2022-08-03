#!/bin/bash
# generates a linux AppImage and moves to the build directory.

set -ex

appdir="AppDir"
rm -rf "build-native/$appdir"
mkdir -p "build-native/$appdir"
meson install --destdir="${appdir}" -C build-native

if [ -z "${VERSION}" ]; then
    export VERSION="$(python waf version --revision)"
fi

cd build-native
export LD_LIBRARY_PATH="build/lib:${LD_LIBRARY_PATH}"
linuxdeploy --appimage-extract-and-run \
    --appdir ${appdir} --output appimage \
    --desktop-file="`find "$appdir" -name "net.kushview.element.desktop"`" \
    --icon-file="../data/ElementIcon_256x256.png" \
    --icon-filename="net.kushview.element" \
    --executable="./element"
rm -rf "${appdir}"

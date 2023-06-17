#!/bin/bash
# generates a linux AppImage and moves to the build directory.

set -ex
builddir="$1"
appdir="AppDir"
rm -rf "$builddir/$appdir"
mkdir -p "$builddir/$appdir"
meson install --destdir="${appdir}" -C "$builddir"

if [ -z "${VERSION}" ]; then
    export VERSION="$(python tools/version.py --revision)"
fi

cd "$builddir"
export LD_LIBRARY_PATH="build/lib:${LD_LIBRARY_PATH}"
linuxdeploy --appimage-extract-and-run \
    --appdir ${appdir} --output appimage \
    --desktop-file="`find "$appdir" -name "net.kushview.element.desktop"`" \
    --icon-file="../tools/lindeploy/icon_256x256.png" \
    --icon-filename="net.kushview.element" \
    --executable="./element"
rm -rf "${appdir}"

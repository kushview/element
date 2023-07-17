#!/bin/bash
# generates a linux AppImage and moves to the build directory.

set -ex
builddir="$1"
appdir="AppDir"
rm -rf "$builddir/$appdir"
mkdir -p "$builddir/$appdir"
meson install --destdir="${appdir}" -C "$builddir" --quiet --skip-subprojects

if [ -z "${VERSION}" ]; then
    export VERSION="$(python util/version.py --build)"
fi

cd "$builddir"
export LD_LIBRARY_PATH="${builddir}/${appdir}/usr/lib:${LD_LIBRARY_PATH}"
linuxdeploy --appimage-extract-and-run \
    --appdir ${appdir} --output appimage \
    --desktop-file="`find "$appdir" -name "net.kushview.element.desktop"`" \
    --icon-file="../deploy/linux/icon_256x256.png" \
    --icon-filename="net.kushview.element" \
    --executable="./element"
rm -rf "${appdir}"

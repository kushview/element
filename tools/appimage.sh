#!/bin/bash
# generates a linux AppImage and moves to the build directory.

set -ex

appdir="build/AppDir"
./waf install --destdir="${appdir}"

export VERSION="`python waf version`"
export LD_LIBRARY_PATH="build/lib:${LD_LIBRARY_PATH}"
linuxdeploy --appimage-extract-and-run \
    --appdir ${appdir} --output appimage \
    --desktop-file="build/share/applications/net.kushview.element.desktop" \
    --icon-file="build/share/icons/hicolor/16x16/apps/net.kushview.element.png" \
    --icon-file="build/share/icons/hicolor/32x32/apps/net.kushview.element.png" \
    --icon-file="build/share/icons/hicolor/64x64/apps/net.kushview.element.png" \
    --icon-file="build/share/icons/hicolor/128x128/apps/net.kushview.element.png" \
    --icon-file="build/share/icons/hicolor/256x256/apps/net.kushview.element.png" \
    --icon-file="build/share/icons/hicolor/512x512/apps/net.kushview.element.png" \
    --executable="build/bin/element"
mv *.AppImage build/
rm -rf "${appdir}"

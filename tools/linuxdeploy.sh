#!/bin/bash
set -ex

appdir="build/AppDir"

rm -rf build
./waf configure --prefix=/usr $@
./waf build
./waf install --destdir="${appdir}"

export VERSION="0.46.0b1"
export LD_LIBRARY_PATH="build/lib"

linuxdeploy \
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

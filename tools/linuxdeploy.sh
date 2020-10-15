#!/bin/bash
set -e

appdir="build/AppDir"
rm -rf build
./waf configure --prefix=/usr
./waf clean build
rm -rf ${appdir}
# ./waf install --destdir="`pwd`/${appdir}"

export LD_LIBRARY_PATH="build/lib"
/mnt/workspace/apps/linuxdeploy-x86_64.AppImage \
    --appdir ${appdir} \
    --icon-file="data/ElementIcon_512x512.png" \
    --executable="build/bin/element" \
    --desktop-file="build/net.kushview.element.desktop" \
    --output appimage

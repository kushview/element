#!/bin/bash

set -e

rm -rf *.gz *.app *.dmg

@BINARYCREATOR@ -v -c "@CONFIGFILE@" -p "@PACKAGES@" @INSTALLERBASE@

sed -i '' 's/com.yourcompany.installerbase/net.kushview.element.installer/' "@INSTALLERBASE@.app/Contents/Info.plist"

@MACDEPLOYQT@ @INSTALLERBASE@.app \
    -no-plugins -dmg -always-overwrite -verbose=3 \
    @MACDEPLOYQT_ARGS@

@NOTARYSCRIPT@

rm -rf archives && mkdir archives
cd packages && @ARCHIVEGEN@ -f tar.gz -c 9 \
    ../archives/element-osx.tar.gz \
        net.kushview.element \
        net.kushview.element.lua
@ARCHIVEGEN@ -f tar.gz -c 9 \
    ../archives/element-plugins-osx.tar.gz \
        net.kushview.element.au \
        net.kushview.element.vst \
        net.kushview.element.vst3

cd ../archives
md5 *.* > md5.sum

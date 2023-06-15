#!/bin/bash

set -e

rm -rf *.gz *.app *.dmg

@BINARYCREATOR@ \
    -e "net.kushview.element.win64" -c "@CONFIGFILE@" -p "@PACKAGES@" @INSTALLERBASE@

sed -i '' 's/com.yourcompany.installerbase/net.kushview.element.osx/' "@INSTALLERBASE@.app/Contents/Info.plist"

@MACDEPLOYQT@ @INSTALLERBASE@.app \
    -no-plugins -dmg -always-overwrite -verbose=3 \
    @MACDEPLOYQT_ARGS@

@NOTARYSCRIPT@

rm -rf archives && mkdir archives
cd packages && @ARCHIVEGEN@ -f tar.gz -c 9 \
    ../archives/net.kushview.element.osx.tar.gz net.kushview.element.osx
cd ..
cd archives
md5 *.* > md5.sum

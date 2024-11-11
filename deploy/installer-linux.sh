#!/bin/bash

set -e
rm -rf *.gz *.app *.dmg

@BINARYCREATOR@ -v -c "@CONFIGFILE@" -p "@PACKAGES@" @INSTALLERBASE@.run

rm -rf archives && mkdir archives
cd packages && @ARCHIVEGEN@ -f tar.gz -c 9 \
    ../archives/element-osx.tar.gz \
        net.kushview.element \
        net.kushview.element.lua

if [ -d "net.kushview.element.vst3" ]; then
    @ARCHIVEGEN@ -f tar.gz -c 9 \
        ../archives/element-plugins-osx.tar.gz \
            net.kushview.element.vst \
            net.kushview.element.vst3
fi

cd ../archives

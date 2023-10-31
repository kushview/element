#!/bin/bash

set -e
rm -rf *.gz *.app *.dmg

@BINARYCREATOR@ -v -c "@CONFIGFILE@" -p "@PACKAGES@" @INSTALLERBASE@

rm -rf archives && mkdir archives
cd packages && @ARCHIVEGEN@ -f tar.gz -c 9 \
    ../archives/element-linux.tar.gz \
        net.kushview.element \
        net.kushview.element.lua

# @ARCHIVEGEN@ -f tar.gz -c 9 \
#     ../archives/element-plugins-linux.tar.gz \
#         net.kushview.element.au \
#         net.kushview.element.vst \
#         net.kushview.element.vst3

cd ../archives
md5 *.* > md5.sum

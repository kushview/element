#!/bin/bash
# Builds release versions of element to run in an AppImage on linux
# If you're compiling for a regular linux distro, don't use this 
# script.

set -e

rm -rf build
rm -f .lock*
here="`pwd`"

export CFLAGS="-DEL_APPIMAGE=1"
export CXXFLAGS="-DEL_APPIMAGE=1"
./waf configure --prefix=/usr $@
./waf build -j2
./waf docs

cd "${here}"
sh build/tools/linuxdeploy.sh
exit $?

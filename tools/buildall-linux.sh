#!/bin/bash
# Builds release versions of element to run in an AppImage on linux
# If you're compiling for a regular linux distro, don't use this 
# script.

set -e

rm -rf build
here="`pwd`"

export CFLAGS="-DEL_APPIMAGE=1"
export CXXFLAGS="-DEL_APPIMAGE=1"
./waf configure --prefix=/usr $@
#./waf resave clean build docs
./waf build

# if [ -f "${here}/tools/jucer/Element/Builds/LinuxMakefile" ]; then
#     cd "${here}/tools/jucer/Element/Builds/LinuxMakefile"
#     make clean && rm -rf build && make -j4 CONFIG=Release
# fi

# if [ -f "${here}/tools/jucer/ElementFX/Builds/LinuxMakefile" ]; then
#     cd "${here}/tools/jucer/ElementFX/Builds/LinuxMakefile"
#     make clean && rm -rf build && make -j4 CONFIG=Release
# fi

cd "${here}"

exit $?

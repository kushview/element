#!/bin/sh
# Builds a Linux AppImage



export CC=clang
export CFLAGS="-DEL_APPIMAGE=1"
export CXX=clang++
export CXXFLAGS="-DEL_APPIMAGE=1"
export LD_LIBRARY_PATH="/depends/x86_64-pc-linux-gnu/lib"

export BOOST_ROOT="/depends/x86_64-pc-linux-gnu/include"

nativefiles="--native-file=tools/machine/linux.ini"
builddir="build-docker"

rm -rf build-docker
meson subprojects download
meson subprojects update

set -e
meson setup -Dbuildtype=release $nativefiles $builddir
meson compile -C $builddir
meson test -C $builddir
sh tools/lindeploy/appimage.sh "$builddir"

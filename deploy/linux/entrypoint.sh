#!/bin/sh
# Builds a Linux AppImage

export CC=clang
export CFLAGS="-DEL_APPIMAGE=1"
export CXX=clang++
export CXXFLAGS="-DEL_APPIMAGE=1"
export LD_LIBRARY_PATH="/depends/x86_64-pc-linux-gnu/lib"

export BOOST_ROOT="/depends/x86_64-pc-linux-gnu/include"

nativefiles="--native-file=meson/subs.ini --native-file=meson/linux.ini --native-file=meson/appimage.ini"
builddir="build-docker"

rm -rf build-docker
meson subprojects download
meson subprojects update --reset

set -e
meson setup -Dbuildtype=release $nativefiles $builddir
meson compile -C $builddir -j3
meson test -C $builddir
sh deploy/linux/appimage.sh "$builddir"

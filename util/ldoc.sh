#!/usr/bin/bash
#
# This file executes ldoc to generate documentation for Lua source files.
# It should be ran from the top level source directory.
#
# Usage: sh util/luadoc.sh

outdir="$(pwd)/build/lua-html"
rm -rf "${outdir}"
mkdir -p "${outdir}"

ldoc -f markdown -q -X \
    -c docs/config.ld \
    --multimodule \
    -d "${outdir}" .

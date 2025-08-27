#!/usr/bin/bash
#
# This file executes ldoc to generate documentation for Lua source files.
# It should be ran from the top level source directory.
#
# Usage: sh util/luadoc.sh [output_directory]

outdir="${1:-$(pwd)/build/lua-html}"
mkdir -p "${outdir}"

ldoc -f markdown -q -X \
    -c docs/config.ld \
    --multimodule \
    -d "${outdir}" .

#!/bin/sh
#
# This file executes ldoc to generate documentation for Lua source files.
# It should be ran from the top level source directory.
#
# Usage: sh util/luadoc.sh [output_directory]

srcdir="$(dirname "$(dirname "$(realpath "$0")")")"
outdir="${1:-$(pwd)/build/lua-html}"

set -e

if [ "${outdir#/}" = "${outdir}" ]; then
    echo "Error: Output directory must be an absolute path" >&2
    exit 1
fi

mkdir -p "${outdir}"

cd "${srcdir}" && ldoc -f markdown -q -X \
    -c docs/config.ld \
    --multimodule \
    -d "${outdir}" .

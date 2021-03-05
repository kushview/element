#!/bin/bash
# Copies Lua scripts and modules into a mac bundle

echo "Bundling lua scripts"

set -e

bundle="build/Applications/Element.app"
resources="${bundle}/Contents/Resources"

if [ -d "${bundle}" ]; then
    rm -rf "${resources}/scripts"
    mkdir -p "${resources}/scripts"
    rsync -ar --delete --include="*.lua" --exclude="*.*" \
        scripts/ "${resources}/scripts/"

    rm -rf "${resources}/lua"
    mkdir -p "${resources}/lua"
    rsync -ar --update --include="*.lua" --exclude="*.*" \
        libs/element/lua/  "${resources}/lua/"
    rsync -ar --update --include="*.lua" --exclude="*.*" \
        libs/lua-kv/src/  "${resources}/lua/"
else
    echo "Skipping `basename ${bundle}`: not an app or plugin bundle"
fi

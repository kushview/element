#!/bin/bash
# Copies Lua scripts and modules into a mac bundle

set -e

bundle="${1}"

if [ -z "${bundle}" ]; then
    bundle="build/Applications/Element.app"
fi

resources="${bundle}/Contents/Resources"
scripts="${resources}/Scripts"
modules="${resources}/Modules"
scriptsrc="scripts"
elsrc="libs/lua"
kvsrc="libs/lua-kv/src"

if [ -d "${bundle}" ]; then
    echo "Copying Lua Scripts"
    rm -rf "${scripts}"
    mkdir -p "${scripts}"
    rsync -ar --delete --include="*.lua" --exclude="*.*" "${scriptsrc}/" "${scripts}/"

    echo "Copying Lua Modules"
    rm -rf "${modules}"
    mkdir -p "${modules}"
    rsync -ar --delete --include="*.lua" --exclude="*.*" "${kvsrc}/"  "${modules}/"
    rsync -ar --update --include="*.lua" --exclude="*.*" "${elsrc}/"  "${modules}/"
else
    echo "Skipping `basename ${bundle}`: not an app or plugin bundle"
fi

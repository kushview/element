#!/bin/bash
export LD_LIBRARY_PATH="`pwd`/build/lib"
export LD_LIBRARY_PATH="`pwd`/build/lib"
export LUA_PATH="libs/lua-kv/src/?.lua;libs/element/lua/?.lua"
export LUA_CPATH="build/lib/lua/?.lua"
export ELEMENT_SCRIPTS_PATH="scripts/?.lua"
build/bin/element $@
exit $?

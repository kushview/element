#!/bin/bash
export LD_LIBRARY_PATH="`pwd`/build/lib"
build/bin/lua-el $@
exit $?

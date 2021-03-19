#!/usr/bin/env python
from subprocess import call
from optparse import OptionParser
import os

def set_local_lua_paths():
    os.environ ['LUA_PATH']             = "libs/lua-kv/src/?.lua;libs/element/lua/?.lua"
    os.environ ['LUA_CPATH']            = "build/lib/lua/?.so"
    os.environ ['ELEMENT_SCRIPTS_PATH'] = "scripts/?.lua"

def options():
    parser = OptionParser()
    parser.add_option ("--no-local-lua", action="store_false", dest="local_lua", default=True,
        help="Load system lua modules and scripts instead of in tree")

    (opts, args) = parser.parse_args()
    return opts

def main():
    opts = options()
    if opts.local_lua:
        set_local_lua_paths()

    cmd = ['build/bin/element']
    call (cmd)

if __name__ == '__main__':
    main()

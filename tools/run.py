#!/usr/bin/env python
from subprocess import call
from optparse import OptionParser
import os

def set_intree_lua_paths():
    os.environ ['LUA_PATH']             = "libs/lua-kv/src/?.lua;libs/element/lua/?.lua"
    os.environ ['LUA_CPATH']            = "build/lib/lua/?.lua"
    os.environ ['ELEMENT_SCRIPTS_PATH'] = "scripts/?.lua"

def options():
    parser = OptionParser()
    parser.add_option ("--system-lua",
                       action="store_true", dest="syslua", default=False,
                       help="Load system lua modules and scripts instead of in tree")

    (opts, args) = parser.parse_args()
    return opts

def main():
    opts = options()
    if not opts.syslua:
        set_intree_lua_paths()

    cmd = ['build/bin/element']
    call (cmd)

if __name__ == '__main__':
    main()

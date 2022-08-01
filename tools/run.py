#!/usr/bin/env python
from subprocess import call
from optparse import OptionParser
import os, platform

def element_binary():
    binary = ''
    if 'Darwin' in platform.system():
        binary = 'build/Applications/Element.app/Contents/MacOS/Element'
    elif 'Linux' in platform.system():
        binary = 'build/bin/element'
    if not os.path.exists (binary):
        raise Exception ("Element binary not found: " + binary)
    return binary

def set_local_lua_paths():
    os.environ ['LUA_PATH']             = "libs/lua/?.lua"
    os.environ ['LUA_CPATH']            = "build/lib/lua/?.so"
    os.environ ['ELEMENT_SCRIPTS_PATH'] = "scripts/?.lua"

def options():
    parser = OptionParser()
    parser.add_option ("--no-local-lua", action="store_false", dest="local_lua", default=True,
        help="Load system lua modules and scripts instead of in tree")
    parser.add_option ("--wine", action="store_true", dest="wine", default=False,
        help="Load system lua modules and scripts instead of in tree")

    (opts, args) = parser.parse_args()
    return opts

def main():
    opts = options()
    if opts.local_lua:
        set_local_lua_paths()
    os.environ['LD_LIBRARY_PATH'] = 'build/lib'
    cmd = []
    if opts.wine:
        cmd.append ('wine')
        cmd.append ('build/bin/element')
    else:
        cmd.append (element_binary())
    call (cmd)

if __name__ == '__main__':
    main()

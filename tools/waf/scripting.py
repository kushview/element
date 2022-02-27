#!/bin/bash/env python
# Scripting config

import os

def make_lua_path (paths):
    out = ''
    if len(paths) > 0:
        for path in paths:
            if len (out) > 0: out += ';'
            out += '%s/?.lua' % path
            out += ';%s/?/init.lua' % path
    return out

def make_lua_spath (paths):
    out = ''
    if len(paths) > 0:
        for path in paths:
            if len(out) > 0: out += ';'
            out += '%s/?.lua' % path
    return out

def make_lua_cpath (paths):
    out = ''
    if len(paths) > 0:
        for path in paths:
            if len(out) > 0: out += ';'
            out += '%s/?.so' % path
            out += ';%s/loadall.so' % path
    return out

def options (opt):
    opt.add_option ('--without-lua', default=False, action='store_true', dest='no_lua', \
        help="Build without LUA scripting")
    opt.add_option ('--luadir', default='', type='string', dest='luadir', \
        help="Specify path to install Lua modules")
    opt.add_option ('--scriptsdir', default='', type='string', dest='scriptsdir', \
        help="Specify path to install Lua scripts")

def configure (self):
    self.env.LUADIR = self.options.luadir.strip()
    if len(self.env.LUADIR) <= 0: 
        self.env.LUADIR = os.path.join (self.env.DATADIR, 'modules')
    
    self.env.SCRIPTSDIR = self.options.scriptsdir.strip()
    if len(self.env.SCRIPTSDIR) <= 0: 
        self.env.SCRIPTSDIR = os.path.join (self.env.DATADIR, 'scripts')

    self.env.LUA_PATH_DEFAULT   = make_lua_path ([ self.env.LUADIR ])
    self.env.LUA_CPATH_DEFAULT  = make_lua_cpath ([
        os.path.join (self.env.LIBDIR, 'element/lua')
    ])
    self.env.EL_SPATH_DEFAULT = make_lua_spath ([
        os.path.join (self.env.DATADIR, 'scripts')
    ])

    self.find_program ('ldoc', mandatory=False)
    
    self.env.LUA = not bool (self.options.no_lua)
    if self.env.LUA:
        self.check_cxx (
            msg = "Checking for Lua",
            includes = [
                self.path.find_node ('libs/lua').abspath(),
                self.path.find_node ('libs/lua/src').abspath(),
            ],
            fragment = '''
                #include <sol/forward.hpp>
                int main (int, char**) {
                    using mystate = sol::state;
                    return 0;
                }
            ''',
            mandatory       = True,
            execute         = False,
            define_name     = 'HAVE_LUA'            
        )
        self.env.LUA = bool(self.env.HAVE_LUA)
    self.define ('EL_USE_LUA', self.env.LUA)
    
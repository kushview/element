project     = 'Element'
title       = 'Element Lua Documentation'
description = 'Element Lua Documentation'
file = { 
    'libs/lua-kv/src',
    'src/scripting/LuaBindings.cpp',
    exclude = {
        'libs/lua-kv/src/lua-kv.h'
    } 
}
dir         = 'build/doc/lua'
output      = 'index'
merge       = true

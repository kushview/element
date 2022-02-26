/*
Copyright 2019-2020 Michael Fisher <mfisher@kushview.net>

Permission to use, copy, modify, and/or distribute this software for any 
purpose with or without fee is hereby granted, provided that the above 
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, 
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
PERFORMANCE OF THIS SOFTWARE.
*/

/// MIDI utilities.
// @author Michael Fisher
// @module kv.round

#include "lua-kv.h"
#include <lualib.h>

static int f_float (lua_State* L) {
    if (lua_gettop (L) <= 0) {
        lua_pushnumber (L, 0.0);
        return 1;
    }

    switch (lua_type (L, 1)) {
        case LUA_TNUMBER:
            lua_pushnumber (L, (float) lua_tonumber (L, 1));
            break;
        case LUA_TBOOLEAN:
            lua_pushnumber (L, (float)(lua_Integer) lua_toboolean (L, 1));
            break;
        case LUA_TSTRING: {
            size_t len = 0;
            lua_tolstring (L, 1, &len);
            lua_pushnumber (L, len > 0 ? 1.0 : 0.0);
            break;
        }
        default:
            lua_pushnumber (L, 0.0);
            break;
    }

    return 1;
}

static int f_integer (lua_State* L) {
    switch (lua_type (L, 1)) {
        case LUA_TNUMBER:
            lua_pushinteger (L, (lua_Integer) lua_tonumber (L, 1));
            break;
        case LUA_TBOOLEAN:
            lua_pushinteger (L, (lua_Integer) lua_toboolean (L, 1));
            break;
        default:
            lua_pushinteger (L, 0);
            break;
    }
    return 1;
}

static const luaL_Reg round_f[] = {
    /// Round a number to 32bit precision.
    // @function floats
    // @number input Number to round
    // @treturn number Rounded value
    { "float",  f_float },

    /// Cast a number to an Integer.
    // @function round.integer
    // @number input Number to round
    // @treturn int The converted integer
    { "integer", f_integer },

    { NULL, NULL }
};

LKV_EXPORT
int luaopen_el_round (lua_State* L) {
    luaL_newlib (L, round_f);
    return 1;
}

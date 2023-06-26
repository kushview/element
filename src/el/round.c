// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

/// MIDI utilities.
// @author Michael Fisher
// @module el.round

#include "element/element.h"
#include <lauxlib.h>

static int f_float (lua_State* L)
{
    if (lua_gettop (L) <= 0)
    {
        lua_pushnumber (L, 0.0);
        return 1;
    }

    switch (lua_type (L, 1))
    {
        case LUA_TNUMBER:
            lua_pushnumber (L, (float) lua_tonumber (L, 1));
            break;
        case LUA_TBOOLEAN:
            lua_pushnumber (L, (float) (lua_Integer) lua_toboolean (L, 1));
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

static int f_integer (lua_State* L)
{
    switch (lua_type (L, 1))
    {
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
    { "float", f_float },

    /// Cast a number to an Integer.
    // @function round.integer
    // @number input Number to round
    // @treturn int The converted integer
    { "integer", f_integer },

    { NULL, NULL }
};

EL_PLUGIN_EXPORT
int luaopen_el_round (lua_State* L)
{
    luaL_newlib (L, round_f);
    return 1;
}

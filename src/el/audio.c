// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "element/element.h"
#include <lauxlib.h>
#include <math.h>

#define MINUS_INFINITY_DB -100.0
#define UNITY_GAIN 1.0

static int f_todb (lua_State* L)
{
    int isnum = 0;
    lua_Number gain = lua_tonumberx (L, 1, &isnum);
    if (isnum == 0)
        gain = UNITY_GAIN;
    lua_Number infinity = lua_tonumberx (L, 2, &isnum);
    if (isnum == 0)
        infinity = MINUS_INFINITY_DB;
    lua_pushnumber (L, gain > 0.0 ? fmax (infinity, log10 (gain) * 20.0) : infinity);
    return 1;
}

static int f_togain (lua_State* L)
{
    int isnum = 0;
    lua_Number db = lua_tonumberx (L, 1, &isnum);
    if (isnum == 0)
        db = 1.0;
    lua_Number infinity = lua_tonumberx (L, 2, &isnum);
    if (isnum == 0)
        infinity = MINUS_INFINITY_DB;
    lua_pushnumber (L, db > infinity ? pow (10.f, db * 0.05) : 0.0);
    return 1;
}

static luaL_Reg audio_f[] = {
    { "togain", f_togain },
    { "todb", f_todb },
    { NULL, NULL }
};

EL_PLUGIN_EXPORT
int luaopen_el_audio (lua_State* L)
{
    luaL_newlib (L, audio_f);
    return 1;
}

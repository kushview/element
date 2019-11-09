/*
    This file is part of Element
    Copyright (C) 2019 Kushview, LLC.  All rights reserved.
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "JuceHeader.h"
#include "scripting/Lua.h"
#include "scripting/LuaState.h"

static int modname_test (lua_State *L) {
    DBG("modname: test()");
    return 1;
}

static const luaL_Reg modnamelib[] = {
    "test", modname_test,
    nullptr, nullptr
};

static int luaopen_modname (lua_State* state) {
    luaL_newlib (state, modnamelib);
    return 1; 
}

namespace Element {

LuaState::LuaState()
{
    state = luaL_newstate();
    luaL_openlibs (state);

    luaL_getsubtable (state, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    lua_pushcfunction (state, luaopen_modname);
    lua_setfield (state, -2, "modname");
    lua_pop (state, 1);  // remove PRELOAD table
}

LuaState::~LuaState()
{
    lua_close (state);
    state = nullptr;
}

}

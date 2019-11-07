/*
    This file is part of Element
    Copyright (C) 2018-2019  Kushview, LLC.  All rights reserved.

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

#include "Tests.h"
#include <lua.hpp>

using namespace Element;

class LuaTest : public UnitTestBase
{
public:
    LuaTest() : UnitTestBase ("Lua Basics", "Lua", "lua") {}
    virtual ~LuaTest() { }

    void runTest()
    {
        runSimpleScript();
    }

private:
    void runSimpleScript() 
    {
        // initialization
        auto* lua = luaL_newstate();
        luaL_openlibs (lua);

        // execute script
        String script = "print('Hello World!')";
        int load_stat = luaL_loadbuffer (lua, script.toRawUTF8(), script.length(), script.toRawUTF8());
        lua_pcall (lua, 0, 0, 0);

        // cleanup
        lua_close (lua);
    }

    void callCFunction() {
        
    }
};

static LuaTest sLuaTest;

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

#if 0
#include "scripting/LuaState.h"


using namespace Element;

class LuaEngine
{
public:
    LuaEngine() {}
    ~LuaEngine() {}

    Result execute (const String& block)
    {
        return Result::ok();
    }
};

class LuaTest : public UnitTestBase
{
public:
    LuaTest() : UnitTestBase ("Lua Basics", "Lua", "lua") {}
    virtual ~LuaTest() { }

    void runTest()
    {
        beginTest ("Basic Lua");
        runSimpleScript();
        getGlobalVars();
        callCFunction();
    }

private:
    static int luaSin (lua_State* state) {
        double d = luaL_checknumber (state, 1);
        lua_pushnumber (state, sin (d));
        return 1;  /* number of results */
    }

    void error (lua_State*, const char* e1, const char* e2)
    {
        String msg (e1); msg << ": " << e2;
        expect (false, msg);
    }

    void runSimpleScript() 
    {
        // initialization
        LuaState lua;

        // execute script
        String script = "print('Hello Lua World!')";
        int load_stat = luaL_loadbuffer (lua, script.toRawUTF8(), script.length(), script.toRawUTF8());
        lua_pcall (lua, 0, 0, 0);
    }

    void callCFunction()
    {
        LuaState lua;

        lua_pushcfunction (lua, luaSin);
        lua_setglobal (lua, "mysin");

        String script = 
R"abc(
print (mysin (100));
print ("hello world 2")
print (mysin (200))
print (mysin ('notanumber'))
)abc";
        int load_stat = luaL_loadbuffer (lua, script.toRawUTF8(), script.length(), "cfunc");
        switch (lua_pcall (lua, 0, 0, 0))
        {
            case LUA_ERRRUN:
                DBG("runtime error");
                break;

            case LUA_OK:
                break;

            default:
                DBG("Unknown lua problem");
                break;
        }
    }

    void getGlobalVars()
    {
        LuaState L;

        String script = 
R"abc(
depth = 10000
width = 100
height = 200
)abc";

        if (LUA_OK != luaL_loadbuffer (L, script.toRawUTF8(), script.length(), "globals"))
            return error (L, "Could not load buffer", "");
        if (LUA_OK != lua_pcall (L, 0, 0, 0))
            return error (L, "Could not execute script", "");

        if (LUA_TNUMBER != lua_getglobal (L, "height"))
            return error (L, "not a number", "");
        if (LUA_TNUMBER != lua_getglobal (L, "width"))
            return error (L, "not a number", "");            
        if (LUA_TNUMBER != lua_getglobal (L, "depth"))
            return error (L, "not a number", "");

        auto depth  = (int) lua_tonumber (L, -1);        
        auto width  = (int) lua_tonumber (L, -2);
        auto height = (int) lua_tonumber (L, -3);

        expect (width == 100);
        expect (height == 200);
        expect (depth == 10000);
    }
};

static LuaTest sLuaTest;
#endif

/*
    This file is part of Element
    Copyright (C) 2020  Kushview, LLC.  All rights reserved.

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
#include "scripting/ScriptManager.h"
#include "scripting/LuaBindings.h"
#include "sol/sol.hpp"

using namespace Element;

static const String sScript1 =
R"(
function func1()
    print("hello there 1")
end

function func2()
end
)";

static const String sScript2 =
R"(
function func1()
    print("hello there 2")
end

function func2()
end
)";

static File scriptsDir()
{
    return File::getSpecialLocation (File::invokedExecutableFile)
                        .getParentDirectory().getParentDirectory().getParentDirectory()
                        .getChildFile ("scripts");
}

//=============================================================================
class ScriptManagerTest : public UnitTestBase
{
public:
    ScriptManagerTest ()
        : UnitTestBase ("Script Manager", "Scripting", "manager") { }

    void runTest() override
    {
        // beginTest ("scanning");
        // ScriptManager scripts;
        // scripts.scanDirectory (scriptsDir());

        beginTest ("loading");
        sol::state lua;
        lua.open_libraries();
        Lua::openLibs (lua);
        Element::Lua::openLibs (lua);
        Element::Lua::setWorld (lua, &getWorld());

        auto buffer = scriptsDir().getChildFile("element/sub/test.lua").loadFileAsString();
        auto result = lua.load_buffer (buffer.toRawUTF8(), buffer.length());
        
        switch (result.status()) {
            case sol::load_status::file:
                DBG("sol::load_status::file");
                break;
            case sol::load_status::gc:
                DBG("sol::load_status::gc");
                break;
            case sol::load_status::memory:
                DBG("sol::load_status::memory");
                break;
            case sol::load_status::ok:
                DBG("sol::load_status::ok");
                break;
            case sol::load_status::syntax:
                DBG("sol::load_status::syntax");
                break;
        }

        lua.script (sScript1.toRawUTF8());
        sol::function f = lua["func1"];
        f();

        lua.script (sScript2.toRawUTF8());
        f = lua["func1"];
        f();
        
        // if (result.get_type() == sol::type::none)
        //     Logger::writeToLog ("no value");
        // else
        //     lua["print"] (result);

        lua.collect_garbage();
        Element::Lua::setWorld (lua, nullptr);
        shutdownWorld();
    }
};

static ScriptManagerTest sScriptManagerTest;

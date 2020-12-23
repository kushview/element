/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "gui/widgets/LuaConsole.h"
#include "gui/LookAndFeel.h"
#include "gui/ViewHelpers.h"
#include "scripting/LuaBindings.h"
#include "Commands.h"

#include "sol/sol.hpp"

namespace Element {

//=============================================================================
LuaConsole::LuaConsole()
    : Console ("Lua Console")
{
    setSize (100, 100);
}

LuaConsole::~LuaConsole() { }

void LuaConsole::textEntered (const String& text)
{
    if (text.isEmpty() || env == nullptr)
        return;
    Console::textEntered (text);
    auto e = env->get();
    sol::state_view lua (e.lua_state());
    
    auto gprint = lua["print"];
    lua["print"] = e["print"];

    try
    {
        bool haveReturn = true;
        String buffer = "return "; buffer << text << ";";
        {
            auto loadResult = lua.load_buffer (buffer.toRawUTF8(), buffer.length());
            if (! loadResult.valid() || loadResult.status() != sol::load_status::ok)
            {
                haveReturn = false;
                buffer = text;
            }
        }
        
        auto result = lua.script (buffer.toRawUTF8(), e,
            // [this](lua_State* L, LuaResult pfr) { return errorHandler (L, pfr); },
            "console=", sol::load_mode::text);
        
        if (result.valid())
        {
            if (haveReturn)
                e["print"](result);
        }
        else
        {
            sol::error error = result;
            for (const auto& line : StringArray::fromLines (error.what()))
                addText (line);
        }
        
        if (lastError.isNotEmpty())
            addText (lastError);
    }
    catch (const sol::error& e)
    {
        addText (e.what());
    }
    
    lua["print"] = gprint;
    lastError.clear();
}

void LuaConsole::setEnvironment (ScriptingEngine::Environment* newEnv)
{
    env.reset (newEnv);
    if (env == nullptr)
        return;
    auto e = env->get();
    jassert (e.valid());
    sol::state_view lua (e.lua_state());

    e["os"]["exit"] = sol::overload (
        [this]() { ViewHelpers::invokeDirectly (this, Commands::quit, true); },
        [this](int code)
        {
            JUCEApplication::getInstance()->setApplicationReturnValue (code);
            ViewHelpers::invokeDirectly (this, Commands::quit, true);
        }
    );

    e["clear"] = [this](sol::variadic_args va)
    {
        if (va.size() == 1 && va.get_type(0) == sol::type::boolean)
        {
            clear (va.get<bool> (0));
        }
        else if (va.size() >= 2 && va.get_type(0) == sol::type::boolean && va.get_type(1) == sol::type::boolean)
        {
            clear (va.get<bool> (0), va.get<bool> (1));
        }
        else
        {
            clear();
        }
    };

    e.set_function ("print", [this](sol::variadic_args va)
    {
        auto e = env->get();
        String msg;
        for (auto v : va)
        {
            if (sol::type::string == v.get_type())
            {
                msg << v.as<const char*>() << " ";
                continue;
            }

            sol::function ts = e["tostring"];
            if (ts.valid())
            {
                sol::object str = ts ((sol::object) v);
                if (str.valid())
                    if (const char* sstr = str.as<const char*>())
                        msg << sstr << "  ";
            }
        }
        
        if (msg.isNotEmpty())
        {
            addText (msg.trimEnd());
            MessageManager::getInstance()->runDispatchLoopUntil (4);
        }
    });

    lua.script ("require('el.script').exec('console')", e);
}

LuaConsole::LuaResult LuaConsole::errorHandler (lua_State* L, LuaResult pfr)
{
    sol::error err = pfr;
    lastError = err.what();
    return pfr;
}

}

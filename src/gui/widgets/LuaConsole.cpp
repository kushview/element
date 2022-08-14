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
#include "scripting/bindings.hpp"
#include "commands.hpp"

#include "sol/sol.hpp"

namespace Element {

//=============================================================================
LuaConsole::LuaConsole()
    : Console ("Lua Console")
{
    setSize (100, 100);
    startTimer (200);
}

LuaConsole::~LuaConsole() {}

void LuaConsole::textEntered (const String& text)
{
    if (text.isEmpty() || ! env.valid())
        return;
    Console::textEntered (text);
    auto& e = env;
    sol::state_view lua (e.lua_state());

    auto gprint = lua["print"];
    lua["print"] = e["print"];

    try
    {
        bool haveReturn = true;
        String buffer = "return ";
        buffer << text << ";";
        {
            auto loadResult = lua.load_buffer (buffer.toRawUTF8(), buffer.length());
            if (! loadResult.valid() || loadResult.status() != sol::load_status::ok)
            {
                haveReturn = false;
                buffer = text;
            }
        }

        auto result = lua.script (buffer.toRawUTF8(), e, "console=", sol::load_mode::text);

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
    } catch (const sol::error& e)
    {
        addText (e.what());
    }

    lua["print"] = gprint;
    lastError.clear();
}

void LuaConsole::setEnvironment (const sol::environment& _env)
{
    env = _env;
    auto& e = env;
    jassert (e.valid());
    sol::state_view lua (e.lua_state());

    e["os"]["exit"] = sol::overload (
        [this]() { ViewHelpers::invokeDirectly (this, Commands::quit, true); },
        [this] (int code) {
            JUCEApplication::getInstance()->setApplicationReturnValue (code);
            ViewHelpers::invokeDirectly (this, Commands::quit, true);
        });

    e["clear"] = [this] (sol::variadic_args va) {
        if (va.size() == 1 && va.get_type (0) == sol::type::boolean)
        {
            clear (va.get<bool> (0));
        }
        else if (va.size() >= 2 && va.get_type (0) == sol::type::boolean && va.get_type (1) == sol::type::boolean)
        {
            clear (va.get<bool> (0), va.get<bool> (1));
        }
        else
        {
            clear();
        }
    };

    e.set_function ("print", [this] (sol::variadic_args va) {
        auto& e = env;
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
            printMessages.add (msg.trimEnd());
        }
    });

    try
    {
        auto result = lua.safe_script ("require('el.script').exec('console', _ENV)", e);
        if (! result.valid())
        {
            sol::error error = result;
            for (const auto& line : StringArray::fromLines (error.what()))
                addText (line);
        }
    } catch (const sol::error& e)
    {
        addText (e.what());
    }
}

void LuaConsole::timerCallback()
{
    if (! printMessages.isEmpty())
    {
        const int block = jmax (1, printMessages.size() / 4);
        const int count = jmin (block, printMessages.size());

        if (count > 0)
        {
            addText (printMessages.joinIntoString ("\n", 0, count));
            printMessages.removeRange (0, count);
        }

        startTimerHz (50);
    }
    else
    {
        startTimer (jmin (250, getTimerInterval() + 10));
    }
}

} // namespace Element

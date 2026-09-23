// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/luaconsole.hpp"
#include <element/ui/style.hpp>
#include "ui/viewhelpers.hpp"
#include "scripting/bindings.hpp"
#include <element/ui/commands.hpp>

#include "sol/sol.hpp"
#include "luascripts.hpp"

namespace element {
using namespace juce;

//=============================================================================
LuaConsole::LuaConsole()
    : Console ("Lua Console")
{
    setSize (100, 100);
    startTimer (200);
}

LuaConsole::~LuaConsole()
{
    if (engine != nullptr)
        engine->consoleHistory() = getHistory();

    // Drop anything in the environment that captured `this`. The environment
    // outlives this component; reads fall back to the globals.
    if (env.valid())
    {
        env["print"] = sol::lua_nil;
        env["clear"] = sol::lua_nil;
        env["os"] = sol::lua_nil;
    }
}

void LuaConsole::initialize (ScriptingEngine& e)
{
    engine = &e;
    env = engine->consoleEnvironment();
    setHistory (engine->consoleHistory());
    installEnvironment();
    runPrelude();
}

void LuaConsole::textEntered (const String& text)
{
    if (text.isEmpty() || engine == nullptr || ! env.valid())
        return;
    Console::textEntered (text);
    addResultText (engine->execute (text, env));
}

void LuaConsole::addResultText (const juce::Result& result)
{
    if (result.wasOk())
        return;
    for (const auto& line : StringArray::fromLines (result.getErrorMessage()))
        addText (line);
}

void LuaConsole::installEnvironment()
{
    jassert (env.valid());
    sol::state_view lua (env.lua_state());

    // A console-local `os` so overriding `exit` does not touch the global table.
    sol::table os = lua.create_table();
    os[sol::metatable_key] = lua.create_table_with ("__index", lua.globals()["os"]);
    os["exit"] = sol::overload (
        [this]() { ViewHelpers::invokeDirectly (this, Commands::quit, true); },
        [this] (int code) {
            JUCEApplication::getInstance()->setApplicationReturnValue (code);
            ViewHelpers::invokeDirectly (this, Commands::quit, true);
        });
    env["os"] = os;

    env["clear"] = [this] (sol::variadic_args va) {
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

    env.set_function ("print", [this] (sol::variadic_args va) {
        sol::state_view state (va.lua_state());
        String msg;
        for (auto v : va)
        {
            if (sol::type::string == v.get_type())
            {
                msg << v.as<const char*>() << " ";
                continue;
            }

            sol::function ts = state["tostring"];
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
            const ScopedLock sl (printLock);
            printMessages.add (msg.trimEnd());
        }
    });
}

void LuaConsole::runPrelude()
{
    // The prelude defines `console`; its presence means it already ran in
    // this (persistent) environment.
    if (env["console"].valid())
        return;
    const auto code = String::fromUTF8 (scripts::console_lua, scripts::console_luaSize);
    addResultText (engine->execute (code, env, "console.lua"));
}

void LuaConsole::timerCallback()
{
    StringArray pending;
    {
        const ScopedLock sl (printLock);
        if (! printMessages.isEmpty())
        {
            const int block = jmax (1, printMessages.size() / 4);
            const int count = jmin (block, printMessages.size());
            pending.addArray (printMessages, 0, count);
            printMessages.removeRange (0, count);
        }
    }

    if (! pending.isEmpty())
    {
        addText (pending.joinIntoString ("\n"));
        startTimerHz (50);
    }
    else
    {
        startTimer (jmin (250, getTimerInterval() + 10));
    }
}

} // namespace element

// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>

#include "scripting.hpp"
#include "sol/sol.hpp"
#include "luascripts.hpp"
#include "testutil.hpp"

using namespace element;
using namespace juce;
namespace et = element::test;

namespace {

/** A console environment whose `print` collects into a StringArray. */
struct ConsoleFixture {
    ConsoleFixture()
        : engine (et::context()->scripting()),
          env (engine.consoleEnvironment())
    {
        env.set_function ("print", [this] (sol::variadic_args va) {
            sol::state_view lua (va.lua_state());
            sol::function tostring = lua["tostring"];
            String line;
            for (auto v : va)
                line << tostring (v).get<std::string>() << " ";
            printed.add (line.trimEnd());
        });
    }

    ~ConsoleFixture()
    {
        env["print"] = sol::lua_nil;
    }

    ScriptingEngine& engine;
    sol::environment env;
    StringArray printed;
};

} // namespace

BOOST_AUTO_TEST_SUITE (ConsoleTests)

BOOST_AUTO_TEST_CASE (ExpressionIsPrinted)
{
    ConsoleFixture fix;
    auto result = fix.engine.execute ("1 + 1", fix.env);
    BOOST_REQUIRE_MESSAGE (result.wasOk(), result.getErrorMessage().toStdString());
    BOOST_REQUIRE_EQUAL (fix.printed.size(), 1);
    BOOST_REQUIRE_EQUAL (fix.printed[0].toStdString(), "2");
}

BOOST_AUTO_TEST_CASE (StatementIsNotPrinted)
{
    ConsoleFixture fix;
    auto result = fix.engine.execute ("local a = 1", fix.env);
    BOOST_REQUIRE (result.wasOk());
    BOOST_REQUIRE (fix.printed.isEmpty());
}

BOOST_AUTO_TEST_CASE (EnvironmentPersists)
{
    {
        ConsoleFixture fix;
        BOOST_REQUIRE (fix.engine.execute ("consoletests_x = 5", fix.env).wasOk());
    }
    {
        ConsoleFixture fix;
        BOOST_REQUIRE (fix.engine.execute ("consoletests_x", fix.env).wasOk());
        BOOST_REQUIRE_EQUAL (fix.printed.size(), 1);
        BOOST_REQUIRE_EQUAL (fix.printed[0].toStdString(), "5");
    }

    // Writes stay in the environment, not in the globals.
    sol::state_view lua (et::context()->scripting().getLuaState());
    BOOST_REQUIRE (! lua["consoletests_x"].valid());
}

BOOST_AUTO_TEST_CASE (SyntaxErrorFails)
{
    ConsoleFixture fix;
    auto result = fix.engine.execute ("local = ", fix.env);
    BOOST_REQUIRE (result.failed());
    BOOST_REQUIRE (result.getErrorMessage().contains ("console"));
    BOOST_REQUIRE (fix.printed.isEmpty());
}

BOOST_AUTO_TEST_CASE (RuntimeErrorHasTraceback)
{
    ConsoleFixture fix;
    auto result = fix.engine.execute ("local function boom() error ('kaboom') end boom()", fix.env);
    BOOST_REQUIRE (result.failed());
    const auto msg = result.getErrorMessage();
    BOOST_REQUIRE_MESSAGE (msg.contains ("kaboom"), msg.toStdString());
    BOOST_REQUIRE_MESSAGE (msg.contains ("stack traceback"), msg.toStdString());
}

BOOST_AUTO_TEST_CASE (ReturnCodeUsesEnvironmentPrint)
{
    ConsoleFixture fix;
    BOOST_REQUIRE (fix.engine.execute ("print ('hello', 2)", fix.env).wasOk());
    BOOST_REQUIRE_EQUAL (fix.printed.size(), 1);
    BOOST_REQUIRE_EQUAL (fix.printed[0].toStdString(), "hello 2");
}

BOOST_AUTO_TEST_CASE (PreludeLoadsFromBinaryData)
{
    ConsoleFixture fix;
    const auto prelude = String::fromUTF8 (scripts::console_lua, scripts::console_luaSize);
    auto result = fix.engine.execute (prelude, fix.env, "console.lua");
    BOOST_REQUIRE_MESSAGE (result.wasOk(), result.getErrorMessage().toStdString());

    BOOST_REQUIRE (fix.env["console"].get_type() == sol::type::table);
    BOOST_REQUIRE (fix.env["session"].get_type() == sol::type::function);
    BOOST_REQUIRE (fix.env["Context"].get_type() == sol::type::table);
    BOOST_REQUIRE (fix.env["command"].get_type() == sol::type::table);
    // el.command generates SHOW_ABOUT etc. from Commands::toString.
    BOOST_REQUIRE (fix.env["command"]["SHOW_ABOUT"].get_type() == sol::type::number);

    // The command manager is reached through el.Context, not a global.
    BOOST_REQUIRE (fix.engine.execute ("Context.instance():commands() ~= nil", fix.env).wasOk());
    BOOST_REQUIRE_EQUAL (fix.printed.size(), 1);
    BOOST_REQUIRE_EQUAL (fix.printed[0].toStdString(), "true");
    fix.printed.clear();

    // console.log goes to the environment's print, not stdout.
    BOOST_REQUIRE (fix.engine.execute ("console.log ('via console')", fix.env).wasOk());
    BOOST_REQUIRE_EQUAL (fix.printed.size(), 1);
    BOOST_REQUIRE_EQUAL (fix.printed[0].toStdString(), "via console");

    // session() resolves the live session through el.Context.
    BOOST_REQUIRE (fix.engine.execute ("session().name", fix.env).wasOk());
    BOOST_REQUIRE_EQUAL (fix.printed.size(), 2);
}

BOOST_AUTO_TEST_CASE (ScriptExecRaisesOnMissingScript)
{
    ConsoleFixture fix;
    auto result = fix.engine.execute ("require ('el.script').exec ('does-not-exist', _ENV)", fix.env);
    BOOST_REQUIRE (result.failed());
    BOOST_REQUIRE (fix.printed.isEmpty());
}

BOOST_AUTO_TEST_SUITE_END()

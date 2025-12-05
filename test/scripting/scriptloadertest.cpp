// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <boost/test/unit_test.hpp>
#include <element/gzip.hpp>

#include "luatest.hpp"
#include "scripting/dspscript.hpp"
#include "scripting/scriptloader.hpp"
#include "utils.hpp"

using namespace element;

static const String sSyntaxError = R"(--- comment
this should trigger a syntax error
)";

static const String sExceptionError = R"(
    local obj = {}
    obj:nil_function()
)";

static const String sAnonymous = R"(
    local msg = "anon"
    testvalue = msg
    return msg
)";

#include <boost/test/unit_test.hpp>
#include "luatest.hpp"

#include "scripting/dspscript.hpp"
#include "luascripts.hpp"

static auto ampScript()
{
    return juce::String::fromUTF8 (scripts::amp_lua, scripts::amp_luaSize);
}

BOOST_AUTO_TEST_SUITE (ScriptLoaderTest)

BOOST_AUTO_TEST_CASE (LoadMethod)
{
    LuaFixture lua;
    auto loader = std::make_unique<ScriptLoader> (lua.luaState());
    BOOST_REQUIRE_MESSAGE (! loader->isLoaded(), "Should not be loaded");
    BOOST_REQUIRE (! loader->hasError());
    loader->load (ampScript());
    BOOST_REQUIRE_MESSAGE (loader->isLoaded(), "Should be loaded");
    BOOST_REQUIRE_MESSAGE (loader->getName().toLowerCase() == "amp", loader->getName().toStdString());
    BOOST_REQUIRE_MESSAGE (loader->getType() == "DSP", loader->getType());
    BOOST_REQUIRE_MESSAGE (loader->getAuthor() == "Michael Fisher", loader->getAuthor());
    auto amp = loader->call();
    BOOST_REQUIRE (amp.valid() && amp.get_type() == sol::type::table);
    loader.reset();
}

BOOST_AUTO_TEST_CASE (LoadCtor)
{
    LuaFixture lua;
    auto loader = std::make_unique<ScriptLoader> (lua.luaState(), ampScript());
    BOOST_REQUIRE_MESSAGE (loader->isLoaded() && ! loader->hasError(),
                           loader->getErrorMessage().toStdString());
    loader.reset();
}

BOOST_AUTO_TEST_CASE (LoadWithError)
{
    LuaFixture lua;
    auto loader = std::make_unique<ScriptLoader> (lua.luaState(), sSyntaxError);
    BOOST_REQUIRE_MESSAGE (loader->hasError(), loader->getErrorMessage().toStdString());

    auto result = loader->call();
    BOOST_REQUIRE_MESSAGE (! result.valid(), sol::type_name (lua.luaState(), result.get_type()));
    BOOST_REQUIRE (loader->getErrorMessage().containsIgnoreCase ("error"));
    DBG (loader->getErrorMessage());
}

BOOST_AUTO_TEST_CASE (Exceptions)
{
    LuaFixture lua;
    auto loader = std::make_unique<ScriptLoader> (lua.luaState());
    loader->load (sExceptionError);
    loader->call();
    BOOST_REQUIRE (loader->getErrorMessage().isNotEmpty());
    BOOST_REQUIRE (loader->hasError());
}

BOOST_AUTO_TEST_CASE (Environment)
{
    LuaFixture fix;
    sol::state_view lua (fix.luaState());

    auto loader = std::make_unique<ScriptLoader> (lua);
    loader->load (sAnonymous);
    auto env = sol::environment (lua, sol::create, lua.globals());
    env["testvalue"] = true;
    sol::object a = env["testvalue"], b = lua["testvalue"];
    BOOST_REQUIRE (a != b);
    BOOST_REQUIRE (a.get_type() == sol::type::boolean);
    BOOST_REQUIRE (b.get_type() == sol::type::lua_nil);

    // environment inside lua
    env["testvalue"] = 95.0;
    lua["testvalue"] = "hello world";
    a = env["testvalue"], b = lua["testvalue"];
    BOOST_REQUIRE (a.get_type() == sol::type::number);
    BOOST_REQUIRE (b.get_type() == sol::type::string);
    lua.script ("expect (testvalue == 'hello world')");
    lua.script ("expect (testvalue == 95)", env);

    sol::function call = loader->caller();
    auto renv = sol::get_environment (call);
    env.set_on (call);
    renv = sol::get_environment (call);
    BOOST_REQUIRE (renv.valid());
    call();
    lua.script ("expect (testvalue == 'anon')", renv);
    lua.script ("expect (testvalue == 'hello world')");
}

BOOST_AUTO_TEST_CASE (ReturnValue)
{
    LuaFixture fix;
    sol::state_view lua (fix.luaState());

    auto loader = std::make_unique<ScriptLoader> (lua);
    loader->load (sAnonymous);
    sol::object obj = loader->call();
    BOOST_REQUIRE_MESSAGE (obj.is<std::string>(), "not a string");
    BOOST_REQUIRE (obj.as<std::string>() == "anon");
}

BOOST_AUTO_TEST_CASE (Base64Encode)
{
    String urlStr = "base64://";
    urlStr << Util::toBase64 (sExceptionError);
    juce::URL url (urlStr);
    const auto decoded = Util::fromBase64 (url.toString (false).replace ("base64://", ""));
    BOOST_REQUIRE_MESSAGE (decoded.trim() == sExceptionError.trim(),
                           decoded.toStdString());
}

BOOST_AUTO_TEST_CASE (GZipEncode)
{
    String urlStr = "gzip://";
    urlStr << element::gzip::encode (sExceptionError);
    juce::URL url (urlStr);
    const auto decompressed = gzip::decode (url.toString (false).replace ("gzip://", ""));
    BOOST_REQUIRE (sExceptionError.trim() == decompressed.trim());
}

BOOST_AUTO_TEST_SUITE_END()

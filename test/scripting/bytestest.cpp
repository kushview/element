// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <boost/test/unit_test.hpp>

#include "luatest.hpp"
#include "scripting/dspscript.hpp"
#include "scripting/scriptloader.hpp"
#include "testutil.hpp"

using namespace element;

BOOST_AUTO_TEST_SUITE (BytesTest)

BOOST_AUTO_TEST_CASE (Basics)
{
    LuaFixture fix;
    sol::state_view lua (fix.luaState());
    auto script = fix.readSnippet ("test_bytes.lua");
    BOOST_REQUIRE (! script.isEmpty());
    try {
        lua.safe_script (script.toRawUTF8(), "[test:bytes]");
    } catch (const std::exception& e) {
        BOOST_REQUIRE_MESSAGE (false, e.what());
    }
}

BOOST_AUTO_TEST_SUITE_END()

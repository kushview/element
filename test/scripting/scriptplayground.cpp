// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <element/script.hpp>

#include "sol/sol.hpp"
#include "testutil.hpp"

using namespace element;
namespace et = element::test;
using namespace juce;

BOOST_AUTO_TEST_SUITE (ScriptPlayground)

BOOST_AUTO_TEST_CASE (Referencing)
{
    sol::state lua;
    lua.open_libraries (sol::lib::base);
    int value = 100;
    lua["value"] = value;
    value = 101;
    auto lval = lua.get<int> ("value");
    BOOST_REQUIRE_NE (value, lval);
}

BOOST_AUTO_TEST_SUITE_END()

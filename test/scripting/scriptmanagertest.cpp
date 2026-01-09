// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <boost/test/unit_test.hpp>
#include "scripting/scriptmanager.hpp"
#include "scripting/bindings.hpp"
#include "sol/sol.hpp"

#include "testutil.hpp"

namespace et = element::test;

BOOST_AUTO_TEST_SUITE (ScriptManagerTest)

BOOST_AUTO_TEST_CASE (ScanDirectory)
{
    element::ScriptManager scripts;
    auto d = et::sourceRoot().getChildFile ("scripts");
    scripts.scanDirectory (d);
    BOOST_REQUIRE_EQUAL (scripts.getNumScripts(), 8);
}

BOOST_AUTO_TEST_SUITE_END()

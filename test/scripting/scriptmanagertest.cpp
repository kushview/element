// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include "scripting/scriptmanager.hpp"

#include "testutil.hpp"

namespace et = element::test;

BOOST_AUTO_TEST_SUITE (ScriptManagerTest)

BOOST_AUTO_TEST_CASE (ScanDirectory)
{
    element::ScriptManager scripts;
    auto d = et::sourceRoot().getChildFile ("scripts");
    scripts.scanDirectory (d);

    // Counts every script that parses with a non-empty @type (ScriptInfo::valid),
    // regardless of type — not just dsp/dspui.
    BOOST_REQUIRE_EQUAL (scripts.getNumScripts(), 13);
}

BOOST_AUTO_TEST_SUITE_END()

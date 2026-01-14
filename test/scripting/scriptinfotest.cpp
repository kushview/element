// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <element/script.hpp>

#include "testutil.hpp"

using namespace element;
namespace et = element::test;
using namespace juce;

//=============================================================================
const static String sDummyScript =
    R"(---  A Dummy script.
-- @script %NAME%
-- @author %AUTHOR%
-- @description A super cool script
-- @type %KIND%
}
)";

const static String sErrorScript =
    R"(
require ("fake module")
some error in the script
)";

const static String sMissingInfo =
    R"(
---
-- Missing Info
-- 
)";

static String toString (const ScriptInfo& desc)
{
    String result;
    result << juce::newLine << "ScriptInfo:" << juce::newLine << " name\t\t = " << desc.name << juce::newLine << " type\t\t = " << desc.type << juce::newLine << " author\t\t = " << desc.author << juce::newLine << " description\t = " << desc.description << juce::newLine << " source\t\t = " << desc.code << juce::newLine;
    return result;
}

BOOST_AUTO_TEST_SUITE (ScriptInfoTest)

BOOST_AUTO_TEST_CASE (ParseBuffer)
{
    auto info = ScriptInfo::parse (sDummyScript
                                       .replace ("%NAME%", "Dummy")
                                       .replace ("%KIND%", "el.DSP")
                                       .replace ("%AUTHOR%", "Michael R. Fisher"));

    BOOST_REQUIRE (info.valid());
    BOOST_REQUIRE (info.name == "Dummy");
    BOOST_REQUIRE (info.type == "DSP");
    BOOST_REQUIRE (info.author == "Michael R. Fisher");
    BOOST_REQUIRE (info.code.isEmpty());
    BOOST_REQUIRE (info.description.isNotEmpty());
}

BOOST_AUTO_TEST_CASE (ParseFile)
{
    const auto sfile = et::sourceRoot().getChildFile ("scripts/amp.lua");
    auto info = ScriptInfo::parse (sfile);
    BOOST_REQUIRE (info.valid());
    BOOST_REQUIRE (info.name == "amp");
    BOOST_REQUIRE (info.type == "DSP");
    BOOST_REQUIRE (info.author == "Michael Fisher");
    BOOST_REQUIRE (URL (info.code).getLocalFile() == sfile);
    BOOST_REQUIRE (info.description.isEmpty());
}

BOOST_AUTO_TEST_CASE (ScriptError)
{
    auto info = ScriptInfo::parse (sErrorScript);
    BOOST_REQUIRE (! info.valid());
}

BOOST_AUTO_TEST_CASE (MissingDetails)
{
    auto info = ScriptInfo::parse (sMissingInfo);
    BOOST_REQUIRE (! info.valid());
}

BOOST_AUTO_TEST_SUITE_END()

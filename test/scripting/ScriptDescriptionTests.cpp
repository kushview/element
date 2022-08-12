/*
    This file is part of Element
    Copyright (C) 2020  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <boost/test/unit_test.hpp>
#include "scripting/ScriptDescription.h"

#include "testutil.hpp"

using namespace Element;
namespace et = element::test;

//=============================================================================
const static String sDummyScript = 
R"(---  A Dummy script.
-- @script %NAME%
-- @author %AUTHOR%
-- @description A super cool script
-- @kind %KIND%
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

static String toString (const ScriptDescription& desc) {
    String result;
    result << juce::newLine << "ScriptDescription:" << juce::newLine <<
        " name\t\t = " << desc.name << juce::newLine <<
        " type\t\t = " << desc.type << juce::newLine <<
        " author\t\t = " << desc.author << juce::newLine <<
        " description\t = " << desc.description << juce::newLine <<
        " source\t\t = " << desc.source << juce::newLine;
    return result;
}

BOOST_AUTO_TEST_SUITE(ScriptDescriptionTests)

BOOST_AUTO_TEST_CASE (ParseBuffer) {
    auto info = ScriptDescription::parse (sDummyScript
        .replace ("%NAME%", "Dummy")
        .replace ("%KIND%", "el.DSP")
        .replace ("%AUTHOR%", "Michael R. Fisher"));

    BOOST_REQUIRE (info.isValid());
    BOOST_REQUIRE (info.name   == "Dummy");
    BOOST_REQUIRE (info.type   == "DSP");
    BOOST_REQUIRE (info.author == "Michael R. Fisher");
    BOOST_REQUIRE (info.source.isEmpty());
    BOOST_REQUIRE (info.description.isNotEmpty());
}

BOOST_AUTO_TEST_CASE (ParseFile) {
    const auto sfile = et::getSourceRoot().getChildFile ("scripts/amp.lua");
    auto info = ScriptDescription::parse (sfile);
    BOOST_REQUIRE (info.isValid());
    BOOST_REQUIRE (info.name   == "amp");
    BOOST_REQUIRE (info.type   == "DSP");
    BOOST_REQUIRE (info.author == "Michael Fisher");
    BOOST_REQUIRE (URL(info.source).getLocalFile() == sfile);
    BOOST_REQUIRE (info.description.isEmpty());
}

BOOST_AUTO_TEST_CASE (ScriptError) {
    auto info = ScriptDescription::parse (sErrorScript);
    BOOST_REQUIRE (! info.isValid());
}

BOOST_AUTO_TEST_CASE (MissingDetails) {
    auto info = ScriptDescription::parse (sMissingInfo);
    BOOST_REQUIRE (! info.isValid());
}

BOOST_AUTO_TEST_SUITE_END()

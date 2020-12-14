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

#include "LuaUnitTest.h"
#include "scripting/ScriptDescription.h"

using namespace Element;

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

static void print (const ScriptDescription& desc) {
    DBG (toString (desc));
}

//=============================================================================
class ScriptDescriptionTest : public UnitTestBase
{
public:
    ScriptDescriptionTest ()
        : UnitTestBase ("Script Description", "ScriptDescription", "parse") { }

    void runTest() override
    {
        beginTest ("parse buffer");
        auto info = ScriptDescription::parse (sDummyScript
            .replace ("%NAME%", "Dummy")
            .replace ("%KIND%", "el.DSP")
            .replace ("%AUTHOR%", "Michael R. Fisher"));

        expect (info.isValid(), "Script description not valid");
        expect (info.name   == "Dummy");
        expect (info.type   == "DSP");
        expect (info.author == "Michael R. Fisher");
        expect (info.source.isEmpty());
        expect (info.description.isNotEmpty());

        beginTest ("parse file");
        const auto sfile = File::getCurrentWorkingDirectory().getChildFile ("scripts/amp.lua");
        info = ScriptDescription::parse (sfile);
        expect (info.isValid());
        expect (info.name   == "amp", toString (info));
        expect (info.type   == "DSP", toString (info));
        expect (info.author == "Michael Fisher", toString (info));
        expect (URL(info.source).getLocalFile() == sfile, toString(info));
        expect (info.description.isEmpty(), toString (info));

        beginTest ("script error");
        info = ScriptDescription::parse (sErrorScript);
        expect (! info.isValid(), "Script should be invalid");

        beginTest ("missing details");
        info = ScriptDescription::parse (sMissingInfo);
        expect (! info.isValid(), "Script should be invalid");
    }
};

static ScriptDescriptionTest sScriptDescriptionTest;

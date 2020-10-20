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

#include "Tests.h"
#include "scripting/ScriptDescription.h"
#include "scripting/ScriptManager.h"

using namespace Element;

//=============================================================================
const static String sSingleLineComments = 
R"(--- Example Action Script
-- @name Example Action Script
-- @author Michael R. Fisher
-- hi there, this is some dummy text 
-- that should be skipped

function somefunc()
    print("not parsed")
end
)";

const static String sBlockComments = 
R"(--[[
Example Action Script
@name BLOCK Example Action Script
@author Michael R. Fisher
hi there, this is some dummy text 
that should be skipped
--]]

function somefunc()
    print("not parsed")
end
)";

//=============================================================================
class ScriptDescriptionTest : public UnitTestBase
{
public:
    ScriptDescriptionTest ()
        : UnitTestBase ("Script Description", "Scripting", "description") { }

    void runTest() override
    {
        beginTest ("ScriptDescription::parse");
        auto info = ScriptDescription::parse (sSingleLineComments);
        expect (info.isValid());
        expect (info.author == "Michael R. Fisher");
        
        info = ScriptDescription::parse (sBlockComments);
        expect (info.isValid());
        expect (info.author == "Michael R. Fisher");
    }
};

static ScriptDescriptionTest sScriptDescriptionTest;

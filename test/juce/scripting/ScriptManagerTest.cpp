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
#include "scripting/ScriptManager.h"
#include "scripting/LuaBindings.h"
#include "sol/sol.hpp"

using namespace Element;

//=============================================================================
class ScriptManagerTest : public UnitTestBase
{
public:
    ScriptManagerTest ()
        : UnitTestBase ("Script Manager", "ScriptManager", "manager") { }

    void runTest() override
    {
        beginTest ("scanDefaultLocation");
        ScriptManager scripts;
        scripts.scanDefaultLocation();
        expect (scripts.getNumScripts() == 6, 
                String("Wrong number of default scripts: ") + String (scripts.getNumScripts()));
    }
};

static ScriptManagerTest sScriptManagerTest;

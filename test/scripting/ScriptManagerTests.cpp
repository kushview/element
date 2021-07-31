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
#include "scripting/ScriptManager.h"
#include "scripting/LuaBindings.h"
#include "sol/sol.hpp"

BOOST_AUTO_TEST_SUITE (ScriptManagerTests)

BOOST_AUTO_TEST_CASE (ScanDirectory) {
    Element::ScriptManager scripts;
    auto d = File::getCurrentWorkingDirectory().getChildFile ("scripts");
    scripts.scanDirectory (d);
    BOOST_REQUIRE_EQUAL (scripts.getNumScripts(), 5);
}

BOOST_AUTO_TEST_SUITE_END()

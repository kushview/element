/*
    This file is part of Element
    Copyright (C) 2018-2019  Kushview, LLC.  All rights reserved.

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
#include <luapp/lua.hpp>

using namespace Element;

class LuaTest : public UnitTest
{
public:
    LuaTest() : UnitTest ("Lua", "lua") { }
    virtual ~LuaTest() { }

    void runTest()
    {
        
    }

private:
    void testGmailFilter()
    {
        beginTest ("Gmail Extended Addresses");
        expect (Util::isGmailExtended ("axetota@gmail.com") == false);
        expect (Util::isGmailExtended ("axetota+extended@gmail.com") == true);
        expect (Util::isGmailExtended ("axetota+@gmail.com") == true);
        expect (Util::isGmailExtended ("axe.to.ta@gmail.com") == true);
        expect (Util::isGmailExtended ("info+yadda@yahoo.com") == false);
        expect (Util::isGmailExtended ("info.ya.dda@yahoo.com") == false);
    }

    void testUuid()
    {
        beginTest ("UUID handling");
    }

    void testName()
    {
        beginTest ("getName() == 'Dummy'");
        expect (getName() == "Dummy");
    }

    void testJavascript()
    {
        beginTest ("JS engine adds console.log(...)");
        JavascriptEngine engine;
        auto* object = new DynamicObject();
        object->setMethod ("log", std::bind (&LuaTest::testLog, std::placeholders::_1));
        engine.registerNativeObject (Identifier ("test"), object);
        auto result = engine.execute (R"(
            var console = { log: test.log };
            console.log("Hello World", 123);
        )");
        expect (! result.failed());
    }

    static var testLog (const var::NativeFunctionArgs& args)
    {
        if (args.numArguments == 1)
        {
            Logger::writeToLog (args.arguments->toString());
        }
        else if (args.numArguments > 1)
        {
            for (int i = 0; i < args.numArguments; ++i)
                Logger::writeToLog ((args.arguments + i)->toString());
        }

        return var::undefined();
    }
};

static LuaTest sLuaTest;

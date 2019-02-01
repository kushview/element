/*
    DummyTest.cpp - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
*/

#include "Tests.h"

using namespace Element;

class DummyTest : public UnitTest
{
public:
    DummyTest() : UnitTest ("Dummy", "dummy") { }
    virtual ~DummyTest() { }

    void runTest()
    {
        testName();
        testJavascript();
        testUuid();
    }

private:
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
        object->setMethod ("log", std::bind (&DummyTest::testLog, std::placeholders::_1));
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

static DummyTest sDummyTest;

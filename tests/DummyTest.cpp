/*
    DummyTest.cpp - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
*/

#include "Tests.h"
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Transaction.h"

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
        testGmailFilter();
        testSQLiteCpp();
    }

private:
    void testSQLiteCpp()
    {
        beginTest ("SQLiteCpp");
        
        const File file (DataPath::applicationDataDir().getChildFile("test.db3"));

        try
        {
            SQLite::Database db (file.getFullPathName().toUTF8(), 
                SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

            db.exec ("DROP TABLE IF EXISTS test");

            // Begin transaction
            SQLite::Transaction transaction (db);

            db.exec(R"(
                CREATE TABLE test (
                    id INTEGER PRIMARY KEY,
                    value TEXT, size INTEGER)
                )");

            for (int i = 0; i < 10000; ++i) 
            {
                String s = "INSERT INTO test VALUES (NULL, ";
                s << "\"test " << i << "\", " << i + 100 << ")";
                db.exec (s.toUTF8());
            }

            // Commit transaction
            transaction.commit();

            // Compile a SQL query, containing one parameter (index 1)
            SQLite::Statement query (db, "SELECT * FROM test");
            
            // Loop to execute the query step by step, to get rows of result
            while (query.executeStep())
            {
                // Demonstrate how to get some typed column value
                int         id      = query.getColumn (0);
                const char* value   = query.getColumn (1);
                int         size    = query.getColumn (2);
                
                // DBG("row: " << id << ", " << value << ", " << size);
            }

            expect (true);
        }
        catch (std::exception& e)
        {
            expect (false, e.what());
        }
       
       
        if (file.existsAsFile())
            file.deleteFile();
    }

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

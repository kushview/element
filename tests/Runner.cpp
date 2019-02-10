/*
    Runner.cpp - This file is part of Element
    Copyright (C) 2014-2018  Kushview, LLC.  All rights reserved.
*/

#include "Tests.h"

int main (int argc, char** argv)
{
    MessageManager::getInstance();
    juce::initialiseJuce_GUI();
    UnitTestRunner runner;
    runner.setAssertOnFailure (true);

    if (argc <= 1)
    {
        runner.runAllTests();
    }
    else if (argc == 2 && UnitTest::getAllCategories().contains (String::fromUTF8 (argv[1])))
    {
        runner.runTestsInCategory (String::fromUTF8 (argv[1]));
    }
    else if (argc == 3)
    {
        const String category (String::fromUTF8 (argv[1]));
        const String slug (String::fromUTF8 (argv[2]));
        Array<UnitTest*> testsToRun;
        for (auto* const unitTest : UnitTest::getAllTests())
            if (auto* const test = dynamic_cast<Element::UnitTestBase*> (unitTest))
                if (category == test->getCategory() && slug == test->getSlug())
                    testsToRun.add (unitTest);
        if (testsToRun.isEmpty())
        {
            Logger::writeToLog ("test(s) not found");
        }
        else
        {
            runner.runTests (testsToRun);
        }
    }
    else
    {
        String notfound ("category not found: "); notfound << String::fromUTF8 (argv[1]);
        Logger::writeToLog (notfound);
    }

    int totalFails, totalPass;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* const result = runner.getResult (i);
        totalFails += result->failures;
        totalPass += result->passes;
    }

    juce::shutdownJuce_GUI();
    Logger::writeToLog ("-----------------------------------------------------------------");
    Logger::writeToLog ("Test Results");
    String message = "pass: "; message << totalPass << " fail: " << totalFails;
    Logger::writeToLog (message);
    return totalFails;
}

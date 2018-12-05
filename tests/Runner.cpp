/*
    Runner.cpp - This file is part of Element
    Copyright (C) 2014-2018  Kushview, LLC.  All rights reserved.
*/

#include "Tests.h"

int main (int argc, char** argv)
{
    UnitTestRunner runner;
    runner.setAssertOnFailure (true);

    if (argc <= 1)
    {
        runner.runAllTests();
    }
    else if (UnitTest::getAllCategories().contains (String::fromUTF8 (argv[1])))
    {
        runner.runTestsInCategory (String::fromUTF8 (argv[1]));
    }
    else
    {
        DBG("category not found: " << String::fromUTF8 (argv[1]));
    }

    int totalFails, totalPass;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* const result = runner.getResult (i);
        totalFails += result->failures;
        totalPass += result->passes;
    }

    juce::shutdownJuce_GUI();
    DBG("pass: " << totalPass << " fail: " << totalFails);
    return totalFails;
}

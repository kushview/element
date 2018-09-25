/*
    Runner.cpp - This file is part of Element
    Copyright (C) 2014-2018  Kushview, LLC.  All rights reserved.
*/

#include "Tests.h"

int main (int argc, char** argv)
{
    UnitTestRunner runner;
    runner.setAssertOnFailure (false);

    if (argc <= 1)
        runner.runAllTests();
    else if (UnitTest::getAllCategories().contains (String::fromUTF8 (argv[1])))
        runner.runTestsInCategory (String::fromUTF8 (argv[1]));
    else
        DBG("category not found: " << String::fromUTF8 (argv[1]));

    int totalFails;
    for (int i = 0; i < runner.getNumResults(); ++i)
        totalFails += runner.getResult(i)->failures;
    juce::shutdownJuce_GUI();
    DBG("total fails: " << totalFails);
    return totalFails;
}

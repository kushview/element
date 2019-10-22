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

    int totalFails = 0, totalPass = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* const result = runner.getResult (i);
        totalFails += result->failures;
        totalPass += result->passes;
    }

    juce::shutdownJuce_GUI();
    Logger::writeToLog ("-----------------------------------------------------------------");
    Logger::writeToLog ("Test Results");
    String message = "pass: "; message << totalPass << " fail: " << totalFails << newLine;
    Logger::writeToLog (message);
    return totalFails;
}

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

static bool copyData()
{
    const auto dataDir      = File::getCurrentWorkingDirectory().getChildFile ("data");
    const auto buildDir     = File::getCurrentWorkingDirectory().getChildFile ("build");
    const auto buildData    = buildDir.getChildFile ("data");
    if (buildData.exists())
        buildData.deleteRecursively();
    return dataDir.copyDirectoryTo (buildDir.getChildFile ("data"));
}


class TestApp : public JUCEApplication,
                public AsyncUpdater
{
    String commandLine;
    std::unique_ptr<Component> comp;

public:
    TestApp() { }
    virtual ~TestApp() { }

    const String getApplicationName()    override      { return "Element Tests"; }
    const String getApplicationVersion() override      { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed()    override      { return false; }

    void handleAsyncUpdate() override
    {
        runUnitTests();
        comp.reset();
    }

    void initialise (const String& cli ) override
    {
        commandLine = cli;
        if (! copyData()) 
        {
            setApplicationReturnValue (100);
            quit();
            return;
        }

        MessageManager::getInstance();
        juce::initialiseJuce_GUI();
        triggerAsyncUpdate();
    }

    void runUnitTests()
    {
        auto opts = StringArray::fromTokens (commandLine, true);
        opts.trim();

        UnitTestRunner runner;
        runner.setAssertOnFailure (true);

    #if JUCE_LINUX
        comp = std::make_unique<Component>();
        comp->setSize (2,2);
        comp->addToDesktop (0);
    #endif

        if (opts.size() <= 0)
        {
            runner.runAllTests();
        }
        else if (opts.size() == 1 && UnitTest::getAllCategories().contains (opts [0]))
        {
            runner.runTestsInCategory (opts[0]);
        }
        else if (opts.size() == 2)
        {
            const String category (opts [0]);
            const String slug (opts [1]);
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
            String notfound ("category not found: "); 
            notfound << opts [0];
            Logger::writeToLog (notfound);
        }

        int totalFails = 0, totalPass = 0;
        for (int i = 0; i < runner.getNumResults(); ++i)
        {
            const auto* const result = runner.getResult (i);
            totalFails += result->failures;
            totalPass += result->passes;
        }

        Logger::writeToLog ("-----------------------------------------------------------------");
        Logger::writeToLog ("Test Results");
        String message = "pass: "; message << totalPass << " fail: " << totalFails << newLine;
        Logger::writeToLog (message);
        setApplicationReturnValue (totalFails);
        systemRequestedQuit();
    }
    
    
    void shutdown() override {}

    void systemRequestedQuit() override
    {
        TestApp::quit();
    }

    void anotherInstanceStarted (const String& commandLine) override
    {
       ignoreUnused (commandLine);
    }

    void resumed() override {}
};

START_JUCE_APPLICATION (TestApp)

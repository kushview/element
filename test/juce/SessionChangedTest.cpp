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

namespace element {

class SessionChangedTest : public UnitTestBase
{
public:
    SessionChangedTest () : UnitTestBase ("Session Changed", "sessionSave") {}
    void initialise() override
    { 
        initializeWorld();
        session = getWorld().getSession();
    }

    void shutdown() override
    {
        session = nullptr;
        shutdownWorld();
    }
    
    void runTest() override
    {
        beginTest("not flagged changed after session open and save");
        const auto sessionFile = getDataDir().getChildFile ("Sessions/Default.els");
        expect (sessionFile.existsAsFile());
        auto* const controller = getServices().findChild<SessionService>();
        controller->openFile (sessionFile);
        runDispatchLoop (40);
        expect (! controller->hasSessionChanged());
        controller->saveSession (false);
        runDispatchLoop (40);
        expect (! controller->hasSessionChanged());
    }

private:
    SessionPtr session;
};

static SessionChangedTest sSessionChangedTest;

}

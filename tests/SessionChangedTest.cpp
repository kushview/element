#include "Tests.h"

namespace Element {

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
        auto* const controller = getAppController().findChild<SessionController>();
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

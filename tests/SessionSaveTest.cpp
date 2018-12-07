#include "Tests.h"

namespace Element {

class SessionSaveTest : public UnitTestBase
{
public:
    SessionSaveTest () : UnitTestBase ("Session Save", "sessionSave") {}
    void initialise() override
    { 
        initializeWorld();
        session = world->getSession();
    }

    void shutdown() override
    {
        session = nullptr;
        shutdownWorld();
    }
    
    void runTest() override
    {

    }

protected:
    void initializeWorld()
    {
        if (world) return;
        world.reset (new Globals ());
        world->setEngine (new AudioEngine (*world));
        app.reset (new AppController (*world));
        app->activate();
    }

    void shutdownWorld()
    {
        if (! world) return;
        app->deactivate();
        app.reset (nullptr);
        world->setEngine (nullptr);
        world.reset (nullptr);
    }

private:
    std::unique_ptr<Globals> world;
    std::unique_ptr<AppController> app;
    SessionPtr session;
};

static SessionSaveTest sSessionSave;

}

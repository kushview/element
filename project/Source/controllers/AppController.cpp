
#include "controllers/AppController.h"
#include "controllers/EngineController.h"
#include "controllers/GuiController.h"

#include "gui/GuiApp.h"
#include "Globals.h"
#include "Messages.h"

namespace Element {

AppController::AppController (Globals& g)
    : world (g)
{
    gui = GuiApp::create (g, *this);
    addChild (new GuiController());
    addChild (new EngineController());
    g.getCommandManager().registerAllCommandsForTarget (this);
    g.getCommandManager().setFirstCommandTarget (this);
}

AppController::~AppController() { }

void AppController::run()
{
    activate();
    getGlobals().getAudioEngine()->activate();
    gui->run();
}

void AppController::handleMessage (const Message& msg)
{
    bool handled = true;
    
    if (const auto* m = dynamic_cast<const LoadPluginMessage*> (&msg)) {
        if (auto* ec = findChild<EngineController>())
            ec->addPlugin (m->description);
    } else {
        handled = false;
    }
    
    if (! handled)
    {
        DBG("[EL] AppController: unhandled Message received");
    }
}
    
ApplicationCommandTarget* AppController::getNextCommandTarget()
{
    return gui.get();
}

void AppController::getAllCommands (Array<CommandID>&) { }
void AppController::getCommandInfo (CommandID, ApplicationCommandInfo&) { }
bool AppController::perform (const InvocationInfo&)
{
    return false;
}

}


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
    g.getCommands().registerAllCommandsForTarget (this);
    g.getCommands().setFirstCommandTarget (this);
}

AppController::~AppController() { }

void AppController::run()
{
    gui->run();
}

void AppController::handleMessage (const Message& msg)
{
    bool handled = true;

    if (const auto* m = dynamic_cast<const LoadPluginMessage*> (&msg)) {
        DBG("LOad it!!!! " << m->desc.fileOrIdentifier);
    } else {
        handled = false;
    }
    
    if (! handled)
    {
        DBG("[EL] AppController: unhandled message received");
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

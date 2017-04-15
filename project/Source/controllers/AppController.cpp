
#include "controllers/AppController.h"
#include "controllers/EngineController.h"
#include "gui/GuiApp.h"
#include "Globals.h"

namespace Element {

AppController::AppController (Globals& g)
    : world (g)
{
    gui = GuiApp::create (g);
    addChild (new EngineController());
    g.getCommands().registerAllCommandsForTarget (this);
}

AppController::~AppController() { }

void AppController::run()
{
    gui->run();
}
    
ApplicationCommandTarget* AppController::getNextCommandTarget()
{
    return nullptr;
}

void AppController::getAllCommands (Array<CommandID>&)
{

}

void AppController::getCommandInfo (CommandID, ApplicationCommandInfo&)
{

}

bool AppController::perform (const InvocationInfo&)
{
    return false;
}

}

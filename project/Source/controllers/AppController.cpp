
#include "controllers/AppController.h"
#include "controllers/EngineController.h"
#include "Globals.h"

namespace Element {

AppController::AppController (Globals& g)
    : world (g)
{
    addChild (new EngineController());
    g.getCommands().registerAllCommandsForTarget (this);
}

AppController::~AppController() { }

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

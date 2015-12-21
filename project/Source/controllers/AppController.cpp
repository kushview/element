
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

AppController::~AppController()
{

}

ApplicationCommandTarget* AppController::getNextCommandTarget()
{
    return nullptr;
}

void AppController::getAllCommands (Array<CommandID>& commands)
{

}

void AppController::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{

}

bool AppController::perform (const InvocationInfo& info)
{
    return false;
}

}

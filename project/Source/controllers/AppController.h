
#ifndef ELEMENT_APP_CONTROLLER_H
#define ELEMENT_APP_CONTROLLER_H

#include "controllers/Controller.h"
#include "CommandManager.h"

namespace Element {
class Globals;

class AppController :  public Controller,
                       protected ApplicationCommandTarget
{
public:
    AppController (Globals&);
    ~AppController();

    CommandManager& getCommandManager() { return commands; }
    Globals& getWorld() { return world; }

protected:
    friend class ApplicationCommandTarget;
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands (Array<CommandID>& commands) override;
    void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform (const InvocationInfo& info) override;

private:
    Globals& world;
    CommandManager commands;
};
}

#endif

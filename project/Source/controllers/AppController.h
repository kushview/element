
#ifndef EL_APP_CONTROLLER_H
#define EL_APP_CONTROLLER_H

#include "controllers/Controller.h"
#include "session/CommandManager.h"

namespace Element {
class Globals;
class GuiApp;

class AppController :  public Controller,
                       protected ApplicationCommandTarget,
                       public MessageListener
{
public:
    AppController (Globals&);
    ~AppController();

    inline CommandManager& getCommandManager() { return commands; }
    inline Globals& getWorld() { return getGlobals(); }
    inline Globals& getGlobals() { return world; }
    
    void activate() override { Controller::activate(); }
    void deactivate() override;
    
protected:
    friend class ApplicationCommandTarget;
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands (Array<CommandID>& commands) override;
    void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform (const InvocationInfo& info) override;
    
    void handleMessage (const Message&) override;
    
private:
    friend class Application;
    File lastSavedFile;
    Globals& world;
    CommandManager commands;
    ScopedPointer<GuiApp> gui;
    void run();
};

}

#endif


#pragma once

#include "controllers/Controller.h"
#include "session/CommandManager.h"

namespace Element {

class Globals;
class Settings;
class UnlockStatus;

struct AppMessage;

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
    inline UndoManager& getUndoManager() { return undo; }

    void activate() override;
    void deactivate() override;

    class Child : public Controller,
                  protected ApplicationCommandTarget
    {
    public:
        Child() { }
        virtual ~Child() { }
        
        inline AppController& getAppController()
        {
            auto* const app = dynamic_cast<AppController*> (getRoot());
            jassert (app); // if you hit this then you're probably calling 
                           // this before controller initialization
            return *app;
        }

        Settings& getSettings();
        UnlockStatus& getUnlockStatus();
        Globals& getWorld();

    protected:
        friend class AppController;
        virtual bool handleMessage (const AppMessage&) { return false; }
    
        friend class ApplicationCommandTarget;
        virtual ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
        virtual void getAllCommands (Array<CommandID>&) override {}
        virtual void getCommandInfo (CommandID, ApplicationCommandInfo&) override {}
        virtual bool perform (const InvocationInfo&) override { return false; }
    };
    
    RecentlyOpenedFilesList& getRecentlyOpenedFilesList() { return recentFiles; }

    void checkForegroundStatus();
    
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
    File lastExportedGraph;
    Globals& world;
    CommandManager commands;
    RecentlyOpenedFilesList recentFiles;
    UndoManager undo;
    boost::signals2::connection licenseRefreshedConnection;
    void licenseRefreshed();
    void run();
};

}

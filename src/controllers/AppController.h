
#pragma once

#include "controllers/Controller.h"
#include "session/CommandManager.h"

namespace Element {
class Globals;

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

    class Child : public Controller
    {
    public:
        Child() { }
        virtual ~Child() { }
        Globals& getWorld();
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


#pragma once

#include "controllers/AppController.h"
#include "gui/LookAndFeel.h"
#include "gui/MainWindow.h"
#include "gui/PreferencesComponent.h"
#include "gui/WindowManager.h"
#include "session/CommandManager.h"
#include "session/Node.h"
#include "session/Session.h"
#include "Signals.h"

namespace Element {

class AboutComponent;
class AppController;
class EngineControl;
class Globals;
class ContentComponent;
class MainWindow;
class PluginWindow;
class SessionDocument;

class GuiController : public AppController::Child,
                      private ChangeListener
{
public:
    Signal<void()> nodeSelected;
    GuiController (Globals& w, AppController& a);
    ~GuiController();
    
    void activate() override;
    void deactivate() override;
    bool handleMessage (const AppMessage&) override;
    
    void run();
    CommandManager& commander();
    
    AppController& getAppController() const { return controller; }
    KeyListener* getKeyListener() const;
            
    void closeAllWindows();
    
    MainWindow* getMainWindow() const { return mainWindow.get(); }
    void refreshMainMenu();
    
    void runDialog (const String& uri);
    void runDialog (Component* c, const String& title = String());

    /** Get a reference to Sesison data */
    SessionRef session();
    
    /** Show plugin windows for a node */
    void showPluginWindowsFor (const Node& node, 
                                const bool recursive = true,
                                const bool force = false,
                                const bool focus = false);
    
    /** present a plugin window */
    void presentPluginWindow (const Node& node, const bool focus = false);
    
    /** Sync all UI elements with application/plugin */
    void stabilizeContent();
    
    /** Stabilize Views Only */
    void stabilizeViews();

    bool haveActiveWindows() const;
    
    /* Command manager... */
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands (Array <CommandID>& commands) override;
    void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform (const InvocationInfo& info) override;

    /** Returns the content component for this instance */
    ContentComponent* getContentComponent();
    
    int getNumPluginWindows() const;
    PluginWindow* getPluginWindow (const int window) const;
    PluginWindow* getPluginWindow (const Node& node) const;
    
    /** Close all plugin windows housed by this controller */
    void closeAllPluginWindows (const bool windowVisible = true);
    
    /** Close plugin windows for a Node ID
     
        @param nodeId   The Node to close windows for
        @param visible  The visibility state flag, true indicates the window should be open when loaded
    */
    void closePluginWindowsFor (uint32 nodeId, const bool windowVisible);
    
    /** Close plugin windows for a Node */
    void closePluginWindowsFor (const Node& node, const bool windowVisible);

    /** @internal close a specific plugin window
        PluginWindows call this when they need deleted
        */
    void closePluginWindow (PluginWindow*);
    
    /** Get the look and feel used by this instance */
    Element::LookAndFeel& getLookAndFeel();

    #if EL_RUNNING_AS_PLUGIN
    void clearContentComponent();
    #endif

    // TODO: content manager on selected nodes
    Node getSelectedNode() const    { return selectedNode; }
    // TODO: content manager on selected nodes.
    // WARNING: don't call from outside the main thread.
    void selectNode (const Node& node)
    {
        selectedNode = node;
        nodeSelected();
    }

private:
    AppController& controller;
    Globals& world;
    SessionRef sessionRef;
    OwnedArray<PluginWindow>         pluginWindows;
    ScopedPointer<WindowManager>     windowManager;
    ScopedPointer<MainWindow>        mainWindow;
    ScopedPointer<ContentComponent>  content;
    ScopedPointer<DialogWindow>      about;
    std::unique_ptr<Component>       activation;
    Node selectedNode; // TODO: content manager

    struct KeyPressManager;
    ScopedPointer<KeyPressManager> keys;

    friend class ChangeBroadcaster;
    void changeListenerCallback (ChangeBroadcaster*) override;
    
    void showSplash();
    void toggleAboutScreen();

    void saveProperties (PropertiesFile* props);
};

}

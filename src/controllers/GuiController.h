
#ifndef EL_GUI_CONTROLLER_H
#define EL_GUI_CONTROLLER_H

#include "ElementApp.h"
#include "controllers/AppController.h"
#include "session/Session.h"
#include "gui/LookAndFeel.h"
#include "gui/PreferencesComponent.h"
#include "gui/WindowManager.h"
#include "session/CommandManager.h"
#include "URIs.h"

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
                          public ApplicationCommandTarget,
                          public Timer
    {
    public:
        GuiController (Globals& w, AppController& a);
        ~GuiController();
        
        void activate() override;
        void deactivate() override;
        
       #if EL_RUNNING_AS_PLUGIN
        void clearContentComponent();
       #endif
        
        void run();
        void timerCallback() override;
        CommandManager& commander();
        
        AppController& getAppController() const { return controller; }
        KeyListener* getKeyListener() const;
        
        Globals& globals();
        
        void closeAllWindows();
        void openWindow (Component* c);
        
        MainWindow* getMainWindow() const { return mainWindow.get(); }
        
        void runDialog (const String& uri);
        void runDialog (Component* c, const String& title = String());

        /** Get a reference to Sesison data */
        SessionRef session();
        
        /** Show plugin windows for a node */
        void showPluginWindowsFor (const Node& node, 
                                   const bool recursive = true,
                                   const bool force = false);
        
        /** present a plugin window */
        void presentPluginWindow (const Node& node);
        
        /** Sync UI elements with application/plugin */
        void stabilizeContent();
        
        /* Command manager... */
        ApplicationCommandTarget* getNextCommandTarget() override;
        void getAllCommands (Array <CommandID>& commands) override;
        void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;
        bool perform (const InvocationInfo& info) override;

        /** Returns the content component for this instance */
        ContentComponent* getContentComponent();
        
        int getNumPluginWindows() const;
        PluginWindow* getPluginWindow (const int window) const;
        
        /** Close all plugin windows housed by this controller */
        void closeAllPluginWindows (const bool windowVisible = true);
        
        /** Close plugin windows for a Node ID
         
            @param nodeId   The Node to close windows for
            @param visible  The visibility state flag, true indicates the window should be open when loaded
        */
        void closePluginWindowsFor (uint32 nodeId, const bool windowVisible);
        
        void closePluginWindowsFor (const Node& node, const bool windowVisible);

        /** @internal close a specific plugin window
            PluginWindows call this when they need deleted
          */
        void closePluginWindow (PluginWindow*);
        
        Element::LookAndFeel& getLookAndFeel();
        
    private:
        AppController& controller;
        Globals& world;
        SessionRef sessionRef;
        OwnedArray<PluginWindow>         pluginWindows;
        ScopedPointer<WindowManager>     windowManager;
        ScopedPointer<MainWindow>        mainWindow;
        ScopedPointer<ContentComponent>  content;
        ScopedPointer<AboutComponent>    about;

        struct KeyPressManager;
        ScopedPointer<KeyPressManager> keys;

        void showSplash();
        void toggleAboutScreen();

        void saveProperties (PropertiesFile* props);
    };
}

#endif  // EL_GUI_CONTROLLER_H

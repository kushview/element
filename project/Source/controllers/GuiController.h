
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
        
        void run();
        void timerCallback() override;
        CommandManager& commander();
        
        AppController& getAppController() const { return controller; }
        
        Globals& globals();
        
        void closeAllWindows();
        
        /** Open an application window by uri
         Not fully operable, the widget factory needs implemented first */
        void openWindow (const String& uri);
        void openWindow (Component* c);
        bool isWindowOpen (const String& uri);
        
        MainWindow* getMainWindow() const { return mainWindow.get(); }
        
        void runDialog (const String& uri);
        void runDialog (Component* c, const String& title = String::empty);
        
        /** Open an existing session */
        void openSession();
        
        /** Create a new session */
        void newSession();
        
        /** Save the session with 'save as' abilities if desired */
        void saveSession (bool saveAs = false);
        
        /** Shutdown the gui and application
         This will be called in  JUCEApplication::systemRequestedQuit().  If
         this method returns true a subsequence JUCEApplication::quit() call is
         made, otherwise the app will continue to run un-altered */
        bool shutdownApp();
        
        /** Get a reference to Sesison data */
        SessionRef session();
        
        /** show plugin windows of this node or child nodes */
        void showPluginWindowsFor (const Node& node, const bool recursive = true);
        
        /** present a plugin window */
        void presentPluginWindow (const Node& node);
        
        
        void stabilizeContent();
        
        /* Command manager... */
        ApplicationCommandTarget* getNextCommandTarget() override;
        void getAllCommands (Array <CommandID>& commands) override;
        void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;
        bool perform (const InvocationInfo& info) override;

        ContentComponent* getContentComponent();
        
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
        
        Element::LookAndFeel& getLookAndFeel() { return lookAndFeel; }
        
    private:
        AppController& controller;
        Globals& world;
        SessionRef sessionRef;
        
        OwnedArray<PluginWindow>         pluginWindows;
        ScopedPointer<WindowManager>     windowManager;
        ScopedPointer<MainWindow>        mainWindow;
        ScopedPointer<ContentComponent>  content;
        ScopedPointer<AboutComponent>    about;
        
        Element::LookAndFeel             lookAndFeel;
        
        void showSplash();
        void toggleAboutScreen();
        void saveProperties (PropertiesFile* props);
    };
}

#endif  // EL_GUI_CONTROLLER_H

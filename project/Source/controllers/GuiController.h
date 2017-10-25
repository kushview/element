
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
    class SessionDocument;
    
    class GuiController : public AppController::Child,
                          public ApplicationCommandTarget
    {
    public:
        GuiController (Globals& w, AppController& a);
        ~GuiController();
        
        void activate() override;
        void deactivate() override;
        
        void run();
        
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
        
        void stabilizeContent();
        
        /* Command manager... */
        ApplicationCommandTarget* getNextCommandTarget() override;
        void getAllCommands (Array <CommandID>& commands) override;
        void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;
        bool perform (const InvocationInfo& info) override;

    private:
        AppController& controller;
        Globals& world;
        SessionRef sessionRef;
        ScopedPointer<SessionDocument> sessionDoc;
        Scoped<WindowManager>     windowManager;
        Scoped<MainWindow>        mainWindow;
        Scoped<ContentComponent>  content;
        Scoped<AboutComponent>    about;
        
        Element::LookAndFeel      lookAndFeel;
        
        void showSplash();
        void toggleAboutScreen();
    };
}

#endif  // EL_GUI_CONTROLLER_H

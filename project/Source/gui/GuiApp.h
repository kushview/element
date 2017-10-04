/*
    GuiApp.h - This file is part of Element
    Copyright (C) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_GUIAPP_H
#define ELEMENT_GUIAPP_H

#include "ElementApp.h"
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

    class GuiApp : public ApplicationCommandTarget
    {
    public:
        virtual ~GuiApp();

        static GuiApp* create (Globals&, AppController&);
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

        /** Get the global URI/URID keychain
            Not in use now, but will be utilized for sending/receiving LV2 Atoms
            from the engine (realtime engine notifications) */
        const URIs* uris();

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
        virtual ApplicationCommandTarget* getNextCommandTarget();
        virtual void getAllCommands (Array <CommandID>& commands);
        virtual void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result);
        virtual bool perform (const InvocationInfo& info);

    protected:
        GuiApp (Globals&, AppController&);

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

        class Dispatch;
        ScopedPointer<Dispatch>   dispatch;
        void runDispatch();
        friend class Dispatch;

        void showSplash();
    };

}
    
#endif /* ELEMENT_GUIAPP_H */

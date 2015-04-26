/*
    GuiApp.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef ELEMENT_GUIAPP_H
#define ELEMENT_GUIAPP_H

#include "element/Juce.h"
#include "session/Session.h"
#include "URIs.h"

#include "Preferences.h"
#include "WindowManager.h"

namespace Element {

    class EngineControl;
    class Globals;

namespace Gui {

    class ContentComponent;
    class MainWindow;
    class SessionDocument;

    class CommandManager :  public ApplicationCommandManager
    {
    public:

        CommandManager() { }
        ~CommandManager() { }
    };

    class GuiApp :  public AppController,
                    public ApplicationCommandTarget
    {
    public:

        virtual ~GuiApp();

        static GuiApp* create (Globals& globals);
        void run();

        CommandManager& commander() { return commandManager; }

        Globals& globals();

        /** Open an application window by uri
            Not fully operable, the widget factory needs implemented first */
        void openWindow (const String& uri);

        void openWindow (Component* c);

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

        /* Command manager... */
        virtual ApplicationCommandTarget* getNextCommandTarget();
        virtual void getAllCommands (Array <CommandID>& commands);
        virtual void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result);
        virtual bool perform (const InvocationInfo& info);

    protected:

        GuiApp (WorldBase&);

    private:

        OpenGLContext render;
        SessionRef sessionRef;
        ScopedPointer<SessionDocument> sessionDoc;

        Scoped<WindowManager>     windowManager;
        Scoped<MainWindow>        mainWindow;
        Scoped<ContentComponent>  content;

        CommandManager            commandManager;
        Element::Style       lookAndFeel;

        class Dispatch;
        ScopedPointer<Dispatch>   dispatch;
        void runDispatch();
        friend class Dispatch;

        void showSplash();

    };


}} /* namespace Element::Gui */

#endif /* ELEMENT_GUIAPP_H */

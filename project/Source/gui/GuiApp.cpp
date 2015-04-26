/*
    GuiApp.cpp - This file is part of Element
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


#include "../engine/AudioEngine.h"
#include "../EngineControl.h"
#include "../MediaManager.h"
#include "../Globals.h"

#include "Alerts.h"
#include "ContentComponent.h"
#include "GuiCommon.h"
#include "GuiApp.h"
#include "MainWindow.h"
#include "PluginListWindow.h"
#include "SessionDocument.h"

#include "layouts/NewSessionView.h"

namespace Element {
namespace Gui {


    class GuiApp::Dispatch : public Timer
    {
    public:

        Dispatch (GuiApp& d)
            : app (d)
        {
            setFrequencyMillis (64);
        }

        ~Dispatch()
        {
            stopTimer();
        }

        void setFrequencyMillis (int millis)
        {
            const bool wasRunning = isTimerRunning();

            if (wasRunning)
                stopTimer();

            milliseconds = millis;

            if (wasRunning && milliseconds > 0)
                startTimer (milliseconds);
        }

        void timerCallback () {
            app.runDispatch();
        }

        /** refresh rate in hertz */
        double refreshRate() const
        {
            if (milliseconds <= 0)
                return 0.0f;

            return  1000.f / (double) milliseconds;
        }

    private:

        GuiApp& app;
        int milliseconds;

    };

    GuiApp::GuiApp (WorldBase& world)
        : AppController (world),
          windowManager (nullptr),
          mainWindow (nullptr)
    {
        dispatch = new Dispatch (*this);
        LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);
        windowManager = new WindowManager (*this);
    }

    GuiApp::~GuiApp()
    {       
        PropertiesFile* pf = globals().settings().getUserSettings();
        pf->setValue ("mainWindowState", mainWindow->getWindowStateAsString());

        File f (sessionDoc->getFile());
        if (f.existsAsFile()) {
            pf->setValue ("lastSession", f.getFullPathName());
        }

        render.detach();

        mainWindow->setVisible (false);
        mainWindow->removeFromDesktop();

        mainWindow = nullptr;
        windowManager = nullptr;
        LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    GuiApp*
    GuiApp::create (Globals& g)
    {
        GuiApp* theGui (new GuiApp (g));
        return theGui;
    }

    Globals& GuiApp::globals()
    {
        Globals* g (static_cast<Globals*> (&world()));
        assert (g);
        return *g;
    }

    void
    GuiApp::openWindow (const String& uri)
    {
        if (uri == ELEMENT_PLUGIN_MANAGER) {
            windowManager->push (new PluginListWindow (globals()));
        } else if (uri == ELEMENT_LEGACY_WINDOW) {
            //windowManager->push (new ABeatThangWindow (*this));
        }
    }

    void
    GuiApp::openWindow (Component* c)
    {
        Window* win = new Window (c->getName(), *this);
        win->setContentOwned (c, true);
        windowManager->push (win);
    }

    void
    GuiApp::runDialog (const String& uri)
    {
        if (uri == ELEMENT_PREFERENCES)
        {
            DialogOptions opts;
            opts.content.set (new PreferencesWidget (*this), true);
            opts.dialogTitle = "Preferences";
            opts.componentToCentreAround = (Component*) mainWindow.get();

            if (DialogWindow* dw = opts.create())
                windowManager->push (dw);
        }
    }

    void
    GuiApp::runDialog (Component* c, const String& title)
    {
        DialogOptions opts;
        opts.content.set (c, true);
        opts.dialogTitle = title != String::empty ? title : c->getName();
        opts.componentToCentreAround = (Component*) mainWindow.get();
        if (DialogWindow* dw = opts.create())
            windowManager->push (dw);
    }

    void GuiApp::showSplash() { }
    void GuiApp::runDispatch() { }

    void
    GuiApp::run()
    {
        commander().registerAllCommandsForTarget (JUCEApplication::getInstance());
        commander().registerAllCommandsForTarget (this);

        globals().session().open();
        sessionDoc = new SessionDocument (globals().session());

        content = new ContentComponent (*this);
        content->setSize (800, 600);
        mainWindow = new MainWindow (*this);
        mainWindow->setContentNonOwned (content.get(), true);

        PropertiesFile* pf = globals().settings().getUserSettings();
        mainWindow->restoreWindowStateFromString (pf->getValue ("mainWindowState"));

       #if JUCE_IOS || JUCE_ANDROID
        Desktop& d = Desktop::getInstance();
        d.setKioskModeComponent (mainWindow);
       #endif

        mainWindow->addToDesktop();
        mainWindow->setVisible (true);
        dispatch->startTimer (250);

        File sess (pf->getValue ("lastSession"));
        if (sess.existsAsFile()) {
            sessionDoc->loadFrom (sess, true);
            mainWindow->setName (sessionDoc->getDocumentTitle());
        }
    }



    bool
    GuiApp::shutdownApp()
    {
        if (! sessionDoc->hasChangedSinceSaved())
            return true;

        bool result = true;

        {
            /* - 0 if the third button was pressed (normally used for 'cancel')
               - 1 if the first button was pressed (normally used for 'yes')
               - 2 if the middle button was pressed (normally used for 'no') */

            //int res = Alerts::showYesNoCancel ("Save Session?", );
            int res = AlertWindow::showYesNoCancelBox (AlertWindow::InfoIcon, "Save Session ?",
                                                       "The current session has changes. Would you like to save them?",
                                                       "Save Session", "Just Quit", "Cancel");

            if (res == 2) {
                // Just Quit
            }
            else if (res == 1) {
                // Save Session
                session()->media().closeAllDocumentsUsingSession (*session(), true);
                sessionDoc->save (true, true);
            } else if (res == 0) {
                result = false; // cancel shutdown
            }
        }

        if (result) {
            windowManager->closeAll();
            session()->close();
            session()->clear();
            session()->controller()->clear();
        }

        return result;
    }

    SessionRef
    GuiApp::session()
    {
        if (sessionRef.get() == nullptr)
            sessionRef = globals().session().makeRef();

        return sessionRef;
    }

    const URIs*
    GuiApp::uris()
    {
        Globals* w (dynamic_cast<Globals*> (&world()));
        return w != nullptr ? w->uris.get() : nullptr;
    }


    void
    GuiApp::openSession()
    {
        if (sessionDoc->loadFromUserSpecifiedFile (true)) {
            mainWindow->setName (sessionDoc->getDocumentTitle());
            // content->stabilize();
        }
    }

    void
    GuiApp::newSession()
    {
        if (sessionDoc->hasChangedSinceSaved()) {
            sessionDoc->save (true, true);
            // content->stabilize();
        }

        DialogWindow::LaunchOptions opts;
        opts.dialogBackgroundColour = Colours::darkgrey;
        opts.content.set (new NewSessionView (globals().session()), true);
        opts.dialogTitle = "New Session";
        opts.resizable = false;
        opts.useBottomRightCornerResizer = false;
        opts.useNativeTitleBar = true;
        opts.componentToCentreAround = (Component*) mainWindow.get();
        if (DialogWindow* dw = opts.create())
        {
            windowManager->push (dw);

            sessionDoc->setFile (File::nonexistent);
            globals().session().clear();
            globals().session().open();

            // content->stabilize();
            sessionDoc->setChangedFlag (false);
        }
    }

    void
    GuiApp::saveSession (bool saveAs)
    {
        if (! saveAs) {
            sessionDoc->save (true, true);
        } else {
            sessionDoc->saveAs (File::nonexistent, true, true, true);
        }

        mainWindow->setName (sessionDoc->getDocumentTitle());
        // content->stabilize();
    }

    ApplicationCommandTarget*
    GuiApp::getNextCommandTarget()
    {
        return nullptr;
    }

    void
    GuiApp::getAllCommands (Array <CommandID>& commands)
    {
        Commands::getDevicePadCommands (commands);
        Commands::getDeviceTrackCommands (commands);

        const CommandID cmds[] = {
            Commands::exportAudio,
            Commands::exportMidi,
            Commands::mediaClose,
            Commands::sessionClose,
            Commands::sessionNew,
            Commands::sessionOpen,
            Commands::sessionSave,
            Commands::sessionSaveAs,
            Commands::showAbout,
            Commands::showLegacyView,
            Commands::showPreferences,
            Commands::showPluginManager,
            Commands::mediaSave,
            Commands::transportRewind,
            Commands::transportForward,
            Commands::transportPlay,
            Commands::transportRecord,
            Commands::transportSeekZero,
            Commands::transportStop,
            StandardApplicationCommandIDs::undo,
            StandardApplicationCommandIDs::redo,
            StandardApplicationCommandIDs::cut,
            StandardApplicationCommandIDs::copy,
            StandardApplicationCommandIDs::paste,
            StandardApplicationCommandIDs::selectAll,
        };

        commands.addArray (cmds, numElementsInArray (cmds));
    }

    void GuiApp::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
    {

        if (Commands::devicePadPress <= commandID && (Commands::devicePadPress + 13) > commandID)
        {
            result.setInfo (("Pad Press"), "Triggers sounds.", "Beat Thang Hardware", ApplicationCommandInfo::dontTriggerVisualFeedback);
            result.addDefaultKeypress ('A', ModifierKeys::noModifiers);
        }
        else if (Commands::devicePadRelease <= commandID && (Commands::devicePadRelease + 13) > commandID)
            result.setInfo (("Pad Release"), "Ends playing sounds.", "Beat Thang Hardware", 0);

        if (result.description.isNotEmpty())
                return;

        if (Commands::getDeviceTrackInfo (commandID, result))
            return;

        switch (commandID)
        {
            case Commands::exportAudio:
                result.setInfo ("Export Audio", "Export to an audio file", "Exporting", 0);
                break;
            case Commands::exportMidi:
                result.setInfo ("Exort MIDI", "Export to a MIDI file", "Exporting", 0);
                break;
            case Commands::mediaClose:
                result.setInfo ("Close Media", "Close the current media", "Application", 0);
                break;
            case Commands::sessionClose:
                result.addDefaultKeypress ('w', ModifierKeys::commandModifier);
                result.setInfo ("Close Session", "Close the current session", "Session", 0);
                break;
            case Commands::sessionNew:
                result.addDefaultKeypress ('n', ModifierKeys::commandModifier);
                result.setInfo ("New Session", "Create a new session", "Session", 0);
                break;
            case Commands::sessionOpen:
                result.addDefaultKeypress ('o', ModifierKeys::commandModifier);
                result.setInfo ("Open Session", "Open an existing session", "Session", 0);
                break;
            case Commands::sessionSave:
                //result.addDefaultKeypress ('s', ModifierKeys::commandModifier);
                result.setInfo ("Save Session", "Save the current session", "Session", 0);
                break;
            case Commands::sessionSaveAs:
                result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
                result.setInfo ("Save Session As", "Save the current session with a new name", "Session", 0);
                break;
            case Commands::showPreferences:
                result.addDefaultKeypress (',', ModifierKeys::commandModifier);
                result.setInfo ("Show Preferences", "BTV Preferences", "Application", 0);
                break;
            case Commands::showAbout:
                result.setInfo ("Show About", "About this program", "Application", 0);
                break;
            case Commands::showLegacyView:
                result.setInfo ("Legacy View", "Shows the legacy Beat Thang Virtual GUI", "Interfaces", 0);
                break;
            case Commands::showPluginManager:
                result.setInfo ("Plugin Manager", "Element Plugin Management", "Application", 0);
                break;
            case StandardApplicationCommandIDs::undo:
                result.setInfo ("Undo", "Element Plugin Management", "Application", 0);
                break;
             case StandardApplicationCommandIDs::redo:
                result.setInfo ("Redo", "Element Plugin Management", "Application", 0);
                break;
            case StandardApplicationCommandIDs::cut:
                result.setInfo ("Cut", "Element Plugin Management", "Application", 0);
                break;
             case StandardApplicationCommandIDs::copy:
                result.setInfo ("Copy", "Element Plugin Management", "Application", 0);
                break;
             case StandardApplicationCommandIDs::paste:
                result.setInfo ("Paste", "Element Plugin Management", "Application", 0);
                break;
             case StandardApplicationCommandIDs::selectAll:
                result.setInfo ("Select All", "Element Plugin Management", "Application", 0);
                break;
            case Commands::mediaSave:
                result.addDefaultKeypress ('s', ModifierKeys::commandModifier);
                result.setInfo ("Close Media", "Closes the currently edited object", "Session Media", 0);
                break;
            case Commands::transportRewind:
                result.setInfo ("Rewind", "Transport Rewind", "Playback", 0);
                result.addDefaultKeypress ('j', 0);
                break;
            case Commands::transportForward:
                result.setInfo ("Forward", "Transport Fast Forward", "Playback", 0);
                result.addDefaultKeypress ('l', 0);
                break;
            case Commands::transportPlay:
                result.setInfo ("Play", "Transport Play", "Playback", 0);
                result.addDefaultKeypress (KeyPress::spaceKey, 0);
                break;
            case Commands::transportRecord:
                result.setInfo ("Record", "Transport Record", "Playback", 0);
                break;
            case Commands::transportSeekZero:
                result.setInfo ("Seek Start", "Seek to Beginning", "Playback", 0);
                break;
            case Commands::transportStop:
                result.setInfo ("Stop", "Transport Stop", "Playback", 0);
                break;
        }
    }

    bool GuiApp::perform (const InvocationInfo& info)
    {


        if (Commands::devicePadPress <= info.commandID
                && (Commands::devicePadPress + 13) > info.commandID)
        {
            const uint16 pad = info.commandID - Commands::devicePadPress;
            //ModifierKeys modKeys = ModifierKeys::getCurrentModifiersRealtime();
            //unsigned long modifiers = 13;//kNoModifiers;

            if (info.isKeyDown)
                std::clog << "Pad pressed: " << pad << std::endl;
            else
                std::clog << "Pad released: " << pad << std::endl;
        }

        if (Commands::isDeviceTrackCommand (info.commandID)) {
        //    pattern->setTrackIndex (info.commandID - Commands::deviceTrack);
            return true;
        }

        SessionRef sr (session());

        switch (info.commandID)
        {
            case Commands::mediaSave: {
                //if (MediaManager::Document* doc = sr->media().openFile (sr.get(), pattern->getFile()))
                 //   doc->save();
                return true;
            }
            case Commands::sessionClose:
                return true;
                break;
            case Commands::sessionNew:
                newSession();
                return true;
                break;
            case Commands::sessionOpen:
                openSession();
                return true;
                break;
            case Commands::sessionSave:
            {
                saveSession();
                return true;
                break;
            }
            case Commands::sessionSaveAs:
                saveSession (true);
                return true;
                break;
            case Commands::showAbout:
                return true;
                break;
            case Commands::showLegacyView:
                openWindow (ELEMENT_LEGACY_WINDOW);
                return true;
                break;
            case Commands::showPluginManager:
                openWindow (ELEMENT_PLUGIN_MANAGER);
                return true;
                break;
            case Commands::showPreferences:
                runDialog (ELEMENT_PREFERENCES);
                return true;
            break;
            case Commands::transportRewind:
                break;
            case Commands::transportForward:
                break;
            case Commands::transportPlay:
                sr->testSetPlaying (true);
                break;
            case Commands::transportRecord:
                break;
            case Commands::transportSeekZero:
                break;
            case Commands::transportStop:
                sr->testSetRecording (false);
                sr->testSetPlaying (false);
                break;
        }
        return true;
    }

}}

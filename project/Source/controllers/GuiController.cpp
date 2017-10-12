
#include "gui/PluginWindow.h"
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "engine/AudioEngine.h"
#include "gui/NewSessionView.h"
#include "gui/AboutComponent.h"
#include "gui/Alerts.h"
#include "gui/ContentComponent.h"
#include "gui/GuiCommon.h"
#include "gui/MainWindow.h"
#include "gui/PluginListWindow.h"
#include "gui/SessionDocument.h"
#include "gui/ConnectionGrid.h"
#include "session/MediaManager.h"
#include "session/UnlockStatus.h"

#include "Globals.h"
#include "Settings.h"
#include "Version.h"

namespace Element {

GuiController::GuiController (Globals& w, AppController& a)
    : controller(a), world(w),
      windowManager (nullptr),
      mainWindow (nullptr)
{
    LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);
    windowManager = new WindowManager();
    commander().registerAllCommandsForTarget (this);
}

GuiController::~GuiController()
{
    PropertiesFile* pf = globals().getSettings().getUserSettings();
    pf->setValue ("mainWindowState", mainWindow->getWindowStateAsString());
    
    closeAllWindows();
    mainWindow->setVisible (false);
    mainWindow->removeFromDesktop();
    
    mainWindow = nullptr;
    windowManager = nullptr;
    LookAndFeel::setDefaultLookAndFeel (nullptr);
}

void GuiController::activate()
{
    Controller::activate();
}

void GuiController::deactivate()
{
    PluginWindow::closeAllCurrentlyOpenWindows();
    Controller::deactivate();
}

void GuiController::closeAllWindows()
{
    if (! windowManager)
        return;
    windowManager->closeAll();
}

CommandManager& GuiController::commander() {
    return world.getCommandManager();
}

Globals& GuiController::globals()
{
    return world;
}

void GuiController::openWindow (const String& uri)
{
    if (uri == ELEMENT_PLUGIN_MANAGER)
    {
        for (int i = DocumentWindow::getNumTopLevelWindows(); --i >= 0;) {
            if (PluginListWindow* w = dynamic_cast<PluginListWindow*> (DocumentWindow::getTopLevelWindow (i))) {
                w->closeButtonPressed();
                mainWindow->refreshMenu();
                return;
            }
        }
        
        windowManager->push (new PluginListWindow (globals()));
        mainWindow->refreshMenu();
    }
}

void GuiController::openWindow (Component* c)
{
    Window* win = new Window (c->getName());
    win->setContentOwned (c, true);
    windowManager->push (win);
}

bool GuiController::isWindowOpen (const String&)
{
    for (int i = DocumentWindow::getNumTopLevelWindows(); --i >= 0;)
        if (PluginListWindow* w = dynamic_cast<PluginListWindow*>(DocumentWindow::getTopLevelWindow (i)))
            return w->isShowing();
    return false;
}

void GuiController::runDialog (const String& uri)
{
    if (uri == ELEMENT_PREFERENCES)
    {
        DialogOptions opts;
        opts.content.set (new PreferencesComponent (world), true);
        opts.dialogTitle = "Preferences";
        opts.componentToCentreAround = (Component*) mainWindow.get();
        
        if (DialogWindow* dw = opts.create())
            windowManager->push (dw);
    }
}

void GuiController::runDialog (Component* c, const String& title)
{
    DialogOptions opts;
    opts.content.set (c, true);
    opts.dialogTitle = title != String::empty ? title : c->getName();
    opts.componentToCentreAround = (Component*) mainWindow.get();
    if (DialogWindow* dw = opts.create())
        windowManager->push (dw);
}

void GuiController::showSplash() { }

void GuiController::run()
{
    content = new ContentComponent (controller);
    content->setSize (800, 600);
    mainWindow = new MainWindow (commander());
    mainWindow->setContentNonOwned (content.get(), true);
    mainWindow->centreWithSize (content->getWidth(), content->getHeight());
    PropertiesFile* pf = globals().getSettings().getUserSettings();
    mainWindow->restoreWindowStateFromString (pf->getValue ("mainWindowState"));
    mainWindow->addKeyListener (commander().getKeyMappings());
    mainWindow->addToDesktop();
    mainWindow->setVisible (true);
}

bool GuiController::shutdownApp()
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
        
        if (res == 2)
        {
            // Just Quit
        }
        else if (res == 1)
        {
            sessionDoc->save (true, true);
        }
        else if (res == 0)
        {
            result = false; // cancel shutdown
        }
    }
    
    if (result)
    {
        windowManager->closeAll();
    }
    
    return result;
}

SessionRef GuiController::session()
{
    if (! sessionRef)
        sessionRef = world.getSession();
    return sessionRef;
}

const URIs* GuiController::uris()
{
    return nullptr; jassertfalse;
    //    Globals* w (dynamic_cast<Globals*> (&world()));
    //    return w != nullptr ? w->uris.get() : nullptr;
}

void GuiController::openSession()
{
    if (sessionDoc->loadFromUserSpecifiedFile (true)) {
        mainWindow->setName (sessionDoc->getDocumentTitle());
        content->stabilize();
    }
}

void GuiController::newSession()
{
    if (sessionDoc->hasChangedSinceSaved())
        sessionDoc->save (true, true);
    sessionDoc->setFile (File::nonexistent);
    sessionDoc->setChangedFlag (false);
    content->stabilize();
    mainWindow->setName (session()->getProperty (Slugs::name));
}

void GuiController::saveSession (bool saveAs)
{
    if (! sessionDoc)
        return;
    
    if (! saveAs) {
        sessionDoc->save (true, true);
    } else {
        sessionDoc->saveAs (File::nonexistent, true, true, true);
    }
    
    mainWindow->setName (sessionDoc->getDocumentTitle());
}

ApplicationCommandTarget* GuiController::getNextCommandTarget()
{
    return nullptr;
}

void GuiController::getAllCommands (Array <CommandID>& commands)
{
    Commands::getDevicePadCommands (commands);
    Commands::getDeviceTrackCommands (commands);
    Commands::getApplicationCommands (commands);
}

void GuiController::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
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
            
        case Commands::sessionClose:
            //            result.addDefaultKeypress ('w', ModifierKeys::commandModifier);
            result.setInfo ("Close Session", "Close the current session", "Session", 0);
            break;
        case Commands::sessionNew:
            //            result.addDefaultKeypress ('n', ModifierKeys::commandModifier);
            result.setInfo ("New Session", "Create a new session", "Session", 0);
            break;
        case Commands::sessionOpen:
            //            result.addDefaultKeypress ('o', ModifierKeys::commandModifier);
            result.setInfo ("Open Session", "Open an existing session", "Session", 0);
            break;
        case Commands::sessionSave:
            //            result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Save Session", "Save the current session", "Session", 0);
            break;
        case Commands::sessionSaveAs:
            //            result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Save Session As", "Save the current session with a new name", "Session", 0);
            break;
            
        case Commands::showPreferences:
            result.setInfo ("Show Preferences", "Element Preferences", "Application", 0);
            result.addDefaultKeypress (',', ModifierKeys::commandModifier);
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
        case Commands::checkNewerVersion:
            result.setInfo ("Check For Updates", "Check newer version", "Application", 0);
            break;
            
        case Commands::mediaNew:
            result.addDefaultKeypress ('n', ModifierKeys::commandModifier);
            result.setInfo ("New Media", "Close the current media", "Application", 0);
            break;
        case Commands::mediaClose:
            result.setInfo ("Close Media", "Close the current media", "Application", 0);
            break;
        case Commands::mediaOpen:
            result.addDefaultKeypress ('o', ModifierKeys::commandModifier);
            result.setInfo ("Open Media", "Opens a type of supported media", "Session Media", 0);
            break;
        case Commands::mediaSave:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier);
            result.setInfo ("Close Media", "Saves the currently viewed object", "Session Media", 0);
            break;
        case Commands::mediaSaveAs:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Close Media", "Saves the current object with another name", "Session Media", 0);
            break;
            
        case Commands::signIn:
            result.setInfo ("Sign In", "Saves the current object with another name", "Application", 0);
            break;
        case Commands::signOut:
            result.setInfo ("Sign Out", "Saves the current object with another name", "Application",   0);
            break;
            
        case Commands::quit:
            result.setActive (false);
            result.setInfo("Quit", "Quit the app", "Application", 0);
            result.addDefaultKeypress ('q', ModifierKeys::commandModifier);
            break;
        case Commands::undo:
            result.setInfo ("Undo", "Undo the last operation", "Application", 0);
            break;
        case Commands::redo:
            result.setInfo ("Redo", "Redo the last operation", "Application", 0);
            break;
        case Commands::cut:
            result.setInfo ("Cut", "Cut", "Application", 0);
            break;
        case Commands::copy:
            result.setInfo ("Copy", "Copy", "Application", 0);
            break;
        case Commands::paste:
            result.setInfo ("Paste", "Paste", "Application", 0);
            break;
        case Commands::selectAll:
            result.setInfo ("Select All", "Select all", "Application", 0);
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

bool GuiController::perform (const InvocationInfo& info)
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
    bool result = true;
    switch (info.commandID)
    {
        case Commands::checkNewerVersion: {
            CurrentVersion::checkAfterDelay (14, true);
        } break;
            
        case Commands::mediaSave: {
            //if (MediaManager::Document* doc = sr->media().openFile (sr.get(), pattern->getFile()))
            //   doc->save();
            return true;
        } break;
            
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
        {
            if (! about) {
                about = new AboutComponent();
                about->centreWithSize(about->getWidth(), about->getHeight());
                about->setVisible(true);
                about->addToDesktop(0);
            }
            else if (about->isVisible())
            {
                about->setVisible (false);
                about->removeFromDesktop();
            }
            else
            {
                about->setVisible(true);
                about->addToDesktop(0);
            }
            return true;
            break;
        }
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
            break;
        case Commands::transportRewind:
            break;
        case Commands::transportForward:
            break;
        case Commands::transportPlay:
            break;
        case Commands::transportRecord:
            break;
        case Commands::transportSeekZero:
            break;
        case Commands::quit:
            JUCEApplication::getInstance()->systemRequestedQuit();
            break;
        case Commands::transportStop:
            break;
        default:
            result = false;
    }
    return result;
}

void GuiController::stabilizeContent()
{
    if (this->content) {
        content->stabilize();
    }
}
}

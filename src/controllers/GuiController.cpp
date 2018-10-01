
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "controllers/SessionController.h"
#include "engine/AudioEngine.h"
#include "gui/AboutComponent.h"
#include "gui/ContentComponent.h"
#include "gui/GuiCommon.h"
#include "gui/MainWindow.h"
#include "gui/PluginWindow.h"
#include "gui/SystemTray.h"
#include "gui/VirtualKeyboardView.h"
#include "CapsLock.h"
#include "Version.h"

namespace Element {

struct GlobalLookAndFeel
{
    GlobalLookAndFeel()     { LookAndFeel::setDefaultLookAndFeel (&look); }
    ~GlobalLookAndFeel()    { LookAndFeel::setDefaultLookAndFeel (nullptr); }
    Element::LookAndFeel look;
};

struct GuiController::KeyPressManager : public KeyListener
{
    KeyPressManager (GuiController& g) : owner (g) { }
    ~KeyPressManager() { }

    bool keyPressed (const KeyPress& key, Component* component) override
    {
        bool handled = false;
        if (isCapsLockOn() && isVirtualKeyboardVisible())
            handled = handleVirtualKeyboardPressed (key, component);
        return handled;
    }

    bool keyStateChanged (bool isKeyDown, Component* component) override
    {
        bool handled = false;
        if (isCapsLockOn() && isVirtualKeyboardVisible())
            handled = handleVirtualKeyboardStateChange (isKeyDown, component);
        return handled;
    }

private:
    bool isVirtualKeyboardVisible() const
    {
        if (auto* cc = owner.getContentComponent())
            return cc->isVirtualKeyboardVisible();
        return false;
    }

    VirtualKeyboardView* getVirtualKeyboardView() const
    {
        if (auto* cc = owner.getContentComponent())
            return cc->getVirtualKeyboardView();
        return nullptr;
    }

    bool handleVirtualKeyboardPressed (const KeyPress& key, Component* component)
    {
        if (auto* vcv = getVirtualKeyboardView())
            return vcv->keyPressed (key, component);
        return false;
    }

    bool handleVirtualKeyboardStateChange (bool isKeyDown, Component*)
    {
        if (auto* vcv = getVirtualKeyboardView())
            return vcv->keyStateChanged (isKeyDown);
        return false;
    }

    GuiController& owner;
};

static ScopedPointer<GlobalLookAndFeel> sGlobalLookAndFeel;
static Array<GuiController*> guiInstances;
static std::unique_ptr<SystemTray> sSystemTray;

GuiController::GuiController (Globals& w, AppController& a)
    : AppController::Child(),
      controller(a), world(w),
      windowManager (nullptr),
      mainWindow (nullptr)
{
    keys = new KeyPressManager (*this);
    if (guiInstances.size() <= 0)
        sGlobalLookAndFeel = new GlobalLookAndFeel();
    guiInstances.add (this);
    windowManager = new WindowManager (*this);
    commander().registerAllCommandsForTarget (this);
}

GuiController::~GuiController()
{
    if (sSystemTray != nullptr)
    {
        sSystemTray->removeFromDesktop();
        sSystemTray.reset (nullptr);
    }

    closeAllPluginWindows (true);

    if (mainWindow)
    {
        mainWindow->removeKeyListener (keys);
        keys = nullptr;

        closeAllWindows();
        mainWindow->setVisible (false);
        mainWindow->removeFromDesktop();
        mainWindow = nullptr;
    }
    
    if (windowManager)
    {
        windowManager = nullptr;
    }
    
    if (content)
    {
        content = nullptr;
    }
    
    guiInstances.removeFirstMatchingValue (this);
    if (guiInstances.size() <= 0)
        sGlobalLookAndFeel = nullptr;
}

Element::LookAndFeel& GuiController::getLookAndFeel()
{ 
    jassert (sGlobalLookAndFeel);
    return sGlobalLookAndFeel->look;
}

void GuiController::timerCallback()
{
    
}

void GuiController::saveProperties (PropertiesFile* props)
{
    jassert(props);
    
    if (mainWindow)
    {
        props->setValue ("mainWindowState", mainWindow->getWindowStateAsString());
        props->setValue ("mainWindowFullScreen", mainWindow->isFullScreen());
        props->setValue ("mainWindowVisible", mainWindow->isVisible());
    }

    if (content)
    {
        props->setValue ("lastContentView", content->getMainViewName());
        props->setValue ("navSize",         content->getNavSize());
        props->setValue ("virtualKeyboard", content->isVirtualKeyboardVisible());
        content->saveState (props);
    }
}

void GuiController::activate()
{
    Controller::activate();
    startTimer (1000);
}

void GuiController::deactivate()
{
    stopTimer();

    saveProperties (getWorld().getSettings().getUserSettings());
    
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

void GuiController::openWindow (const String& uri) { }

void GuiController::openWindow (Component* c)
{
    Window* win = new Window (c->getName());
    win->setContentOwned (c, true);
    windowManager->push (win);
}

bool GuiController::isWindowOpen (const String&)
{
    return false;
}

void GuiController::runDialog (const String& uri)
{ 
    if (uri == ELEMENT_PREFERENCES)
    {
        if (auto* const dialog = windowManager->findDialogByName ("PreferencesDialog"))
        {
            if (!dialog->isOnDesktop() || !dialog->isVisible())
            {
                dialog->setVisible (true);
                dialog->addToDesktop();
            }
            dialog->toFront (true);
            return;
        }

        DialogOptions opts;
        opts.content.set (new PreferencesComponent (world, *this), true);
        opts.useNativeTitleBar = true;
        opts.dialogTitle = "Preferences";
        opts.componentToCentreAround = (Component*) mainWindow.get();
        
        if (DialogWindow* dw = opts.create())
        {
            dw->setName ("PreferencesDialog");
            windowManager->push (dw, true);        
        }
    }
}

void GuiController::closePluginWindow (PluginWindow* w) { windowManager->closePluginWindow (w); }
void GuiController::closePluginWindowsFor (uint32 nodeId, const bool visible) { windowManager->closeOpenPluginWindowsFor (nodeId, visible); }
void GuiController::closeAllPluginWindows (const bool visible) { windowManager->closeAllPluginWindows (visible); }

void GuiController::closePluginWindowsFor (const Node& node, const bool visible)
{
    if (! node.isGraph())
        windowManager->closeOpenPluginWindowsFor (node, visible);
}

void GuiController::runDialog (Component* c, const String& title)
{
    DialogOptions opts;
    opts.content.set (c, true);
    opts.dialogTitle = title.isNotEmpty() ? title : c->getName();
    opts.componentToCentreAround = (Component*) mainWindow.get();
    if (DialogWindow* dw = opts.create())
        windowManager->push (dw);
}

void GuiController::showSplash() { }

ContentComponent* GuiController::getContentComponent()
{
    if (! content)
    {
        content = new ContentComponent (controller);
        content->setSize (760, 480);
    }
    
    return content.get();
}

int GuiController::getNumPluginWindows() const
{
    return (nullptr != windowManager) ? windowManager->getNumPluginWindows()
                                      : 0;
}

PluginWindow* GuiController::getPluginWindow (const int window) const
{
    return (nullptr != windowManager) ? windowManager->getPluginWindow (window)
                                      : 0;
}

void GuiController::showPluginWindowsFor (const Node& node, const bool recursive,
                                          const bool force)
{
    if (! node.isGraph())
    {
        if (force || (bool) node.getProperty ("windowVisible", false))
            presentPluginWindow (node);
        return;
    }

    if (node.isGraph() && recursive)
        for (int i = 0; i < node.getNumNodes(); ++i)
            showPluginWindowsFor (node.getNode (i), recursive, force);
}

void GuiController::presentPluginWindow (const Node& node)
{
    if (! windowManager)
        return;

    auto* window = windowManager->getPluginWindowFor (node);
    if (! window)
        window = windowManager->createPluginWindowFor (node);

    if (window != nullptr)
    {
        window->setName (node.getName());
       #if EL_RUNNING_AS_PLUGIN
        // This makes plugin window handling more like the standalone
        // we don't want to modify the existing standalone behavior
        window->setAlwaysOnTop (true);
       #endif
        window->setVisible (true);
        window->toFront (false);
    }
}

void GuiController::run()
{
    mainWindow = new MainWindow (world);
    mainWindow->setContentNonOwned (getContentComponent(), true);
    mainWindow->centreWithSize (content->getWidth(), content->getHeight());
    PropertiesFile* pf = globals().getSettings().getUserSettings();
    mainWindow->restoreWindowStateFromString (pf->getValue ("mainWindowState"));
    mainWindow->addKeyListener (keys);
    mainWindow->addKeyListener (commander().getKeyMappings());
    getContentComponent()->restoreState (pf);
    mainWindow->addToDesktop();

    if (pf->getBoolValue ("mainWindowVisible", true))
    {
        mainWindow->setVisible (true);
        if (pf->getBoolValue ("mainWindowFullScreen"))
            mainWindow->setFullScreen (true);
    }
    
    findSibling<SessionController>()->resetChanges();

   #if ! EL_RUNNING_AS_PLUGIN
    sSystemTray.reset (new SystemTray());
    sSystemTray->setIconImage (
        ImageCache::getFromMemory (BinaryData::ElementIcon_png, BinaryData::ElementIcon_pngSize)    
    );
    sSystemTray->addToDesktop (0);
   #endif
}

bool GuiController::shutdownApp()
{
    jassertfalse;
    return true;
}

SessionRef GuiController::session()
{
    if (! sessionRef)
        sessionRef = world.getSession();
    return sessionRef;
}

void GuiController::openSession()
{
}

void GuiController::newSession()
{
}

void GuiController::saveSession (bool saveAs)
{
}

ApplicationCommandTarget* GuiController::getNextCommandTarget()
{
    return content.get();
}

void GuiController::getAllCommands (Array <CommandID>& commands)
{
    commands.addArray ({
        Commands::showAbout,
		Commands::showPluginManager,
		Commands::showPreferences,
        Commands::showSessionConfig,
        Commands::showGraphConfig,
        Commands::showPatchBay,
        Commands::showGraphEditor,
        Commands::showLastContentView,
        Commands::toggleVirtualKeyboard,
        Commands::rotateContentView,
        Commands::showAllPluginWindows,
        Commands::hideAllPluginWindows,
        Commands::showKeymapEditor
    });
    
    commands.add (Commands::quit);
}

void GuiController::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    typedef ApplicationCommandInfo Info;
    switch (commandID)
    {
        case Commands::exportAudio:
            result.setInfo ("Export Audio", "Export to an audio file", "Session", 0);
            break;
        case Commands::exportMidi:
            result.setInfo ("Exort MIDI", "Export to a MIDI file", "Session", 0);
            break;
        case Commands::importGraph:
            result.setInfo ("Import graph", "Import a graph into current session", "Session", 0);
            break;
        case Commands::exportGraph:
            result.setInfo ("Export current graph", "Export the current graph to file", "Session", 0);
            break;
        case Commands::panic:
            result.addDefaultKeypress ('p', ModifierKeys::altModifier | ModifierKeys::commandModifier);
            result.setInfo ("Panic!", "Sends all notes off to the engine", "Engine", 0);
            break;
            
            // MARK: Session Commands
        case Commands::sessionClose:
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
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier);
            result.setInfo ("Save Session", "Save the current session", Commands::Categories::session, 0);
            break;
        case Commands::sessionSaveAs:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Save Session As", "Save the current session with a new name", Commands::Categories::session, 0);
            break;
        case Commands::sessionAddGraph:
            result.addDefaultKeypress ('n', ModifierKeys::shiftModifier | ModifierKeys::commandModifier);
            result.setInfo ("Add graph", "Add a new graph to the session", Commands::Categories::session, 0);
            break;
        case Commands::sessionDuplicateGraph:
            result.addDefaultKeypress ('d', ModifierKeys::shiftModifier | ModifierKeys::commandModifier);
            result.setInfo ("Duplicate current graph", "Duplicates the currently active graph", Commands::Categories::session, 0);
            break;
        case Commands::sessionDeleteGraph:
            result.addDefaultKeypress (KeyPress::backspaceKey, ModifierKeys::commandModifier);
            result.setInfo ("Delete current graph", "Deletes the current graph", Commands::Categories::session, 0);
            break;
        case Commands::sessionInsertPlugin:
            result.addDefaultKeypress ('p', ModifierKeys::commandModifier);
            result.setInfo ("Insert plugin", "Add a plugin in the current graph", Commands::Categories::session, Info::isDisabled);
            break;
            
        // MARK: Media Commands
        case Commands::mediaNew:
            result.setInfo ("New Media", "Close the current media", "Session", 0);
            break;
        case Commands::mediaClose:
            result.setInfo ("Close Media", "Close the current media", "Session", 0);
            break;
        case Commands::mediaOpen:
            result.setInfo ("Open Media", "Opens a type of supported media", "Session", 0);
            break;
        case Commands::mediaSave:
            result.setInfo ("Save Media", "Saves the currently viewed object", "Session", 0);
            break;
        
        case Commands::mediaSaveAs:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Save Media As", "Saves the current object with another name", "Session", 0);
            break;
            
            // MARK: Show Commands
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
        
        case Commands::showLastContentView:
            result.setInfo ("Last View", "Shows the last content view", "User Interface", 0);
            break;
        case Commands::showSessionConfig:
        {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "SessionSettings") flags |= Info::isTicked;
            result.setInfo ("Session Settings", "Session Settings", "Session", flags);
        } break;
        case Commands::showGraphConfig:
        {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "GraphSettings") flags |= Info::isTicked;
            result.setInfo ("Graph Settings", "Graph Settings", "Session", flags);
        } break;
        
        case Commands::showPatchBay:
        {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "PatchBay") flags |= Info::isTicked;
            result.addDefaultKeypress (KeyPress::F1Key, 0);
            result.setInfo ("Patch Bay", "Show the patch bay", "Session", flags);
        } break;
        
        case Commands::showGraphEditor:
        {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "GraphEditor") flags |= Info::isTicked;
            result.addDefaultKeypress (KeyPress::F2Key, 0);
            result.setInfo ("Graph Editor", "Show the graph editor", "Session", flags);
        } break;
            
        case Commands::toggleVirtualKeyboard:
        {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->isVirtualKeyboardVisible()) flags |= Info::isTicked;
            result.setInfo ("Virtual Keyboard", "Toggle the virtual keyboard", "Session", flags);
        } break;
        
        case Commands::rotateContentView:
            result.addDefaultKeypress ('r', ModifierKeys::commandModifier | ModifierKeys::altModifier);
            result.setInfo ("Rotate View", "Show the graph editor", "Session", 0);
            break;

        case Commands::showAllPluginWindows:
            result.addDefaultKeypress ('w', ModifierKeys::commandModifier | ModifierKeys::altModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Show all plugin windows", "Show all plugins for the current graph.", "Session", 0);
            break;
        case Commands::hideAllPluginWindows:
            result.addDefaultKeypress ('w', ModifierKeys::commandModifier | ModifierKeys::altModifier);
            result.setInfo ("Hide all plugin windows", "Hides all plugins on the current graph.", "Session", 0);
            break;
        case Commands::showKeymapEditor:
            // result.addDefaultKeypress ('w', ModifierKeys::commandModifier | ModifierKeys::altModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Keymap Editor", "Show the keyboard shortcuts and edit them.", "User Interface", 0);
            break;

        case Commands::checkNewerVersion:
            result.setInfo ("Check For Updates", "Check newer version", "Application", 0);
            break;
            
        case Commands::signIn:
            result.setInfo ("Sign In", "Saves the current object with another name", "Application", 0);
            break;
        case Commands::signOut:
            result.setInfo ("Sign Out", "Saves the current object with another name", "Application",   0);
            break;
            
        case Commands::quit:
            result.setInfo ("Quit", "Quit the app", "Application", 0);
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
            result.addDefaultKeypress ('c', ModifierKeys::commandModifier);
            result.setInfo ("Copy", "Copy", "Application", Info::isDisabled);
            break;
        case Commands::paste:
            result.addDefaultKeypress ('p', ModifierKeys::commandModifier);
            result.setInfo ("Paste", "Paste", "Application", Info::isDisabled);
            break;
        case Commands::selectAll:
            result.setInfo ("Select All", "Select all", "Application", 0);
            break;
            
        case Commands::transportRewind:
            result.setInfo ("Rewind", "Transport Rewind", "Engine", 0);
            result.addDefaultKeypress ('j', 0);
            break;
        case Commands::transportForward:
            result.setInfo ("Forward", "Transport Fast Forward", "Engine", 0);
            result.addDefaultKeypress ('l', 0);
            break;
        case Commands::transportPlay:
            result.setInfo ("Play", "Transport Play", "Engine", 0);
            result.addDefaultKeypress (KeyPress::spaceKey, 0);
            break;
        case Commands::transportRecord:
            result.setInfo ("Record", "Transport Record", "Engine", 0);
            break;
        case Commands::transportSeekZero:
            result.setInfo ("Seek Start", "Seek to Beginning", "Engine", 0);
            break;
        case Commands::transportStop:
            result.setInfo ("Stop", "Transport Stop", "Engine", 0);
            break;
    }
}

bool GuiController::perform (const InvocationInfo& info)
{
    bool result = true;
    switch (info.commandID)
    {
        case Commands::showAbout:
            toggleAboutScreen();
            break;

        case Commands::showKeymapEditor:
            content->setMainView ("KeymapEditorView");
            break;
        case Commands::showPluginManager:
            content->setMainView ("PluginManager");
            break;
        case Commands::showPreferences:
            runDialog (ELEMENT_PREFERENCES);
            break;
        case Commands::showSessionConfig:
            content->setMainView ("SessionSettings");
            break;
        case Commands::showGraphConfig:
            content->setMainView ("GraphSettings");
            break;
        case Commands::showPatchBay:
            content->setMainView ("PatchBay");
            break;
        case Commands::showGraphEditor:
            content->setMainView ("GraphEditor");
            break;
        case Commands::toggleVirtualKeyboard:
            content->toggleVirtualKeyboard();
            break;
            
        case Commands::showLastContentView:
            content->backMainView();
            break;
        case Commands::rotateContentView:
            content->nextMainView();
            break;
        
        case Commands::showAllPluginWindows: {
            if (auto s = getWorld().getSession())
                showPluginWindowsFor (s->getActiveGraph(), true, true);
        } break;

        case Commands::hideAllPluginWindows: {
                closeAllPluginWindows (false);
        } break;

        case Commands::quit:
            JUCEApplication::getInstance()->systemRequestedQuit();
            break;
        
        default:
            result = false;
    }
    
    if (result && mainWindow)
    {
        mainWindow->refreshMenu();
    }
    
    return result;
}

void GuiController::stabilizeContent()
{
    if (auto* cc = content.get())
        cc->stabilize();
}

void GuiController::toggleAboutScreen()
{
    if (! about)
    {
        about = new AboutComponent();
        about->centreWithSize (about->getWidth(), about->getHeight());
    }

    jassert (about);

    if (about->isVisible())
    {
        about->setVisible (false);
        about->removeFromDesktop();
    }
    else
    {
        about->setVisible (true);
        about->addToDesktop (0);
       #if EL_RUNNING_AS_PLUGIN
        about->setAlwaysOnTop (true);
       #endif
    }
}

KeyListener* GuiController::getKeyListener() const { return keys.get(); }

#if EL_RUNNING_AS_PLUGIN
void GuiController::clearContentComponent()
{
    if (about)
    {
        about->setVisible (false);
        about->removeFromDesktop();
        about = nullptr;
    }

    jassert(content != nullptr);
    content = nullptr;
}
#endif

}


#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "controllers/SessionController.h"
#include "engine/AudioEngine.h"
#include "gui/AboutComponent.h"
#include "gui/ContentComponent.h"
#include "gui/GuiCommon.h"
#include "gui/MainWindow.h"
#include "gui/PluginWindow.h"
#include "Version.h"

namespace Element {

GuiController::GuiController (Globals& w, AppController& a)
    : AppController::Child(),
      controller(a), world(w),
      windowManager (nullptr),
      mainWindow (nullptr)
{
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

void GuiController::timerCallback()
{
    
}
    
void GuiController::activate()
{
    LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);
    Controller::activate();
    startTimer (1000);
}

void GuiController::deactivate()
{
    stopTimer();
    
    auto* const props (getWorld().getSettings().getUserSettings());
    if (props && content)
    {
        props->setValue ("lastContentView", content->getMainViewName());
        props->setValue ("virtualKeyboard", (bool)content->isVirtualKeyboardVisible());
    }
    
    Controller::deactivate();
}

void GuiController::closeAllWindows()
{
    if (! windowManager)
        return;
    PluginWindow::closeAllCurrentlyOpenWindows();
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
        DialogOptions opts;
        opts.content.set (new PreferencesComponent (world, *this), true);
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
    sessionDoc = new SessionDocument (session());
    content = new ContentComponent (controller);
    content->setSize (800, 600);
    mainWindow = new MainWindow (world);
    mainWindow->setContentNonOwned (content.get(), true);
    mainWindow->centreWithSize (content->getWidth(), content->getHeight());
    PropertiesFile* pf = globals().getSettings().getUserSettings();
    mainWindow->restoreWindowStateFromString (pf->getValue ("mainWindowState"));
    mainWindow->addKeyListener (commander().getKeyMappings());
    mainWindow->addToDesktop();
    mainWindow->setVisible (true);
    findSibling<SessionController>()->resetChanges();
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
                                                   "The current session has changes. Would you like to save it?",
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
    return nullptr;
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
        Commands::toggleVirtualKeyboard,
        Commands::rotateContentView
    });
    
    commands.add (Commands::quit);
}

void GuiController::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    Commands::getCommandInfo (commandID, result);
}

bool GuiController::perform (const InvocationInfo& info)
{
    bool result = true;
    switch (info.commandID)
    {
        case Commands::showAbout:
            toggleAboutScreen();
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
        case Commands::rotateContentView:
            content->nextMainView();
            break;
            
        case Commands::quit:
            JUCEApplication::getInstance()->systemRequestedQuit();
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
    
void GuiController::toggleAboutScreen()
{
    if (! about)
    {
        about = new AboutComponent();
        about->centreWithSize(about->getWidth(), about->getHeight());
        about->setVisible (true);
        about->addToDesktop(0);
    }
    else if (about->isVisible())
    {
        about->setVisible (false);
        about->removeFromDesktop();
    }
    else
    {
        about->setVisible (true);
        about->addToDesktop(0);
    }
}
    
}

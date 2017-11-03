/*
    Main.cpp - This file is part of Element
    Copyright (C) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#include "ElementApp.h"
#include "controllers/AppController.h"
#include "controllers/SessionController.h"
#include "engine/InternalFormat.h"
#include "engine/GraphProcessor.h"
#include "session/DeviceManager.h"
#include "session/PluginManager.h"
#include "session/UnlockStatus.h"
#include "session/GoogleAnalyticsDestination.h"
#include "Commands.h"
#include "DataPath.h"
#include "Globals.h"
#include "Messages.h"
#include "Version.h"
#include "Settings.h"

namespace Element {

class Startup : public ActionBroadcaster,
                private Thread
{
public:
    Startup (Globals& w, const bool useThread = false, const bool splash = false)
        : Thread ("ElementStartup"),
          world (w), usingThread (useThread),
          showSplash (splash),
        isFirstRun (false)
    { }

    ~Startup() { }
    
    void updateSettingsIfNeeded()
    {
        UnlockStatus& status (world.getUnlockStatus());
        Settings& settings (world.getSettings());

        if (! status.isFullVersion())
        {
            auto* props = settings.getUserSettings();
            props->setValue ("clockSource", "internal");
            settings.saveIfNeeded();
        }
    }
    
    void launchApplication()
    {
        updateSettingsIfNeeded();
        setupAnalytics();

        Settings& settings (world.getSettings());
        isFirstRun = !settings.getUserSettings()->getFile().existsAsFile();
        DeviceManager& devices (world.getDeviceManager());
        auto* props = settings.getUserSettings();
        if (ScopedXml dxml = props->getXmlValue ("devices"))
        {
            devices.initialise (16, 16, dxml.get(), true, "default", nullptr);
        }
        else
        {
            devices.initialiseWithDefaultDevices (16, 16);
        }
        
        if (usingThread)
        {
            startThread();
            while (isThreadRunning())
                MessageManager::getInstance()->runDispatchLoopUntil (30);
        }
        else
        {
            if (showSplash)
                (new StartupScreen())->deleteAfterDelay (RelativeTime::seconds(5), true);
            this->run();
        }
    }

    ScopedPointer<AppController> controller;
    
    const bool isUsingThread() const { return usingThread; }

private:
    Globals& world;
    const bool usingThread;
    const bool showSplash;
    bool isFirstRun;
    
    class StartupScreen :  public SplashScreen
    {
    public:
        StartupScreen ()
            : SplashScreen ("Element", 600, 400, true)
        {
            addAndMakeVisible (text);
            text.setText ("Loading Application", dontSendNotification);
            text.setSize (600, 400);
            text.setFont (Font (24.0f));
            text.setJustificationType (Justification::centred);
            text.setColour (Label::textColourId, Colours::white);
        }

        void resized() override
        {
            SplashScreen::resized();
            text.setBounds (getLocalBounds());
        }

        void paint (Graphics& g) override
        {
            SplashScreen::paint (g);
            g.fillAll (Colours::aliceblue);
        }

    private:
        Label text;
    };

    void run() override
    {
        Settings& settings (world.getSettings());
        PluginManager& plugins (world.getPluginManager());
        AudioEnginePtr engine = new AudioEngine (world);
        engine->applySettings (settings);
        world.setEngine (engine); // this will also instantiate the session
        SessionPtr session = world.getSession();
        
        plugins.addDefaultFormats();
        plugins.addFormat (new InternalFormat (*engine));
        plugins.addFormat (new ElementAudioPluginFormat());
        
        if (isFirstRun)
        {
            auto& formats (plugins.formats());
            for (int i = 0; i < formats.getNumFormats(); ++i)
            {
                auto* format = formats.getFormat (i);
                if (! format->canScanForPlugins())
                    continue;
                PluginDirectoryScanner scanner (plugins.availablePlugins(), *format,
                                                format->getDefaultLocationsToSearch(),
                                                true, File::nonexistent, false);
                String name;
                DBG("[EL] Scanning for " << format->getName() << " plugins...");
                while (scanner.scanNextFile (true, name))
                {
                    DBG("[EL]  " << name);
                }
            }
        }
        else
        {
            plugins.restoreUserPlugins (settings);
        }
        
        world.loadModule ("test");
        controller = new AppController (world);
        
        if (usingThread) {
            sendActionMessage ("finishedLaunching");
        }
    }
    
    void setupAnalytics()
    {
        // Add an analytics identifier for the user. Make sure you don't collect
        // identifiable information accidentally if you haven't asked for permission!
        Analytics::getInstance()->setUserId ("annonymous");
        
        StringPairArray userData;
        userData.set ("group", "beta");
        Analytics::getInstance()->setUserProperties (userData);
        Analytics::getInstance()->addDestination (new GoogleAnalyticsDestination());
        Analytics::getInstance()->logEvent ("startup", {});
    }
};

class Application : public JUCEApplication,
                    public ActionListener
{
public:
    Application() { }
    virtual ~Application() { }

    const String getApplicationName()    override      { return ProjectInfo::projectName; }
    const String getApplicationVersion() override      { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed()    override      { return true; }

    void initialise (const String&  commandLine ) override
    {
        if (sendCommandLineToPreexistingInstance())
        {
            quit();
            return;
        }
        
        Logger::writeToLog ("Element v" + getApplicationVersion());
        Logger::writeToLog ("Copyright (c) 2017-2018 Kushview, LLC.  All rights reserved.\n");
        
        initializeModulePath();
        world = new Globals (commandLine);
        world->getUnlockStatus().loadAll();
        launchApplication();
    }
    
    void actionListenerCallback (const String& message) override
    {
        if (message == "finishedLaunching")
            finishLaunching();
    }
    
    void shutdown() override
    {
        if (! world || ! controller)
            return;
        
        Analytics::getInstance()->logEvent ("shutdown", {});
        
        UnlockStatus& status (world->getUnlockStatus());
        status.save();
        
        auto engine (world->getAudioEngine());
        auto& plugins (world->getPluginManager());
        auto& settings (world->getSettings());
        auto* props = settings.getUserSettings();
        
        controller->deactivate();
        
        plugins.saveUserPlugins (settings);
        if (ScopedXml el = world->getDeviceManager().createStateXml())
            props->setValue ("devices", el);
        
        engine = nullptr;
        controller = nullptr;
        world->setEngine (nullptr);
        world->unloadModules();
        world = nullptr;
    }

    void systemRequestedQuit() override
    {
        if (! controller)
        {
            Application::quit();
            return;
        }
        
        auto* sc = controller->findChild<SessionController>();
        
        // - 0 if the third button was pressed ('cancel')
        // - 1 if the first button was pressed ('yes')
        // - 2 if the middle button was pressed ('no')
        
        const int res = !sc->hasSessionChanged() ? 2
            : AlertWindow::showYesNoCancelBox (AlertWindow::WarningIcon,
                                               "Save Session",
                                               "This session may have changes. Would you like to save before exiting?");
        
        if (res == 1)
            sc->saveSession();
        
        if (res != 0)
            Application::quit();
    }

    void anotherInstanceStarted (const String& commandLine) override
    {
        if (! controller)
            return;
        
        if (auto* sc = controller->findChild<SessionController>())
        {
            const auto path = commandLine.unquoted().trim();
            if (File::isAbsolutePath (path))
            {
                const File file (path);
                if (file.hasFileExtension ("els"))
                    sc->openFile (file);
                else if (file.hasFileExtension("elg"))
                    sc->importGraph (file);
            }
        }
    }

    void finishLaunching()
    {
        if (nullptr != controller || nullptr == startup)
            return;
        
        controller = startup->controller.release();
        startup = nullptr;
        controller->run();
        const bool checkUpdatesOnStart = false;
        if (checkUpdatesOnStart)
            CurrentVersion::checkAfterDelay (5000);
        
        if (auto* sc = controller->findChild<SessionController>())
        {
            const auto path = world->cli.commandLine.unquoted().trim();
            if (File::isAbsolutePath (path))
            {
                const File file (path);
                if (file.hasFileExtension ("els"))
                    sc->openFile (File (path));
            }
        }
    }
    
private:
    String launchCommandLine;
    ScopedPointer<Globals>       world;
    ScopedPointer<AppController> controller;
    ScopedPointer<Startup>       startup;
    
    void launchApplication()
    {
        if (nullptr != controller)
            return;
        
        startup = new Startup (*world, false, false);
        startup->addActionListener (this);
        startup->launchApplication();
        if (startup && !startup->isUsingThread())
            finishLaunching();
    }
    
    void initializeModulePath()
    {
        const File path (File::getSpecialLocation (File::invokedExecutableFile));
        File modDir = path.getParentDirectory().getParentDirectory()
                          .getChildFile("lib/element").getFullPathName();
       #if JUCE_DEBUG
        if (! modDir.exists()) {
            modDir = path.getParentDirectory().getParentDirectory()
                         .getChildFile ("modules");
        }
       #endif
        
       #if JUCE_WINDOWS
        String putEnv = "ELEMENT_MODULE_PATH="; putEnv << modDir.getFullPathName();
        putenv (putEnv.toRawUTF8());
       #else
        setenv ("ELEMENT_MODULE_PATH", modDir.getFullPathName().toRawUTF8(), 1);
       #endif
    }
};

}

START_JUCE_APPLICATION (Element::Application)

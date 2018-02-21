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
    
    void launchApplication()
    {
        Settings& settings (world.getSettings());
        isFirstRun = !settings.getUserSettings()->getFile().existsAsFile();
        DataPath path;
        ignoreUnused (path);
        
        updateSettingsIfNeeded();
        setupAnalytics();

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
    friend class Application;
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
    
    void run() override
    {
        Settings& settings (world.getSettings());
        AudioEnginePtr engine = new AudioEngine (world);
        engine->applySettings (settings);
        world.setEngine (engine); // this will also instantiate the session
        SessionPtr session = world.getSession();
        
        setupPlugins();
        
        world.loadModule ("test");
        controller = new AppController (world);
        
        sendActionMessage ("finishedLaunching");
    }
    
    void setupPlugins()
    {
        auto& settings (world.getSettings());
        auto& plugins  (world.getPluginManager());
        auto  engine   (world.getAudioEngine());
        
        plugins.addDefaultFormats();
        plugins.addFormat (new InternalFormat (*engine));
        plugins.addFormat (new ElementAudioPluginFormat (world));
        plugins.restoreUserPlugins (settings);
        plugins.setPropertiesFile (settings.getUserSettings());
        plugins.scanInternalPlugins();
        plugins.searchUnverifiedPlugins();
    }
    
    void setupAnalytics()
    {
       #if 0
        auto* analytics (Analytics::getInstance());
        StringPairArray userData;
        userData.set ("group", "beta");
        analytics->setUserId ("annonymous");
        analytics->setUserProperties (userData);
        analytics->addDestination (new GoogleAnalyticsDestination());
        analytics->logEvent ("startup", {});
       #endif
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

    void initialise (const String& commandLine ) override
    {
        world = new Globals (commandLine);
        world->getUnlockStatus().loadAll();
        initializeModulePath();
        
        if (maybeLaunchSlave (commandLine))
            return;
        
        if (sendCommandLineToPreexistingInstance())
        {
            quit();
            return;
        }
        
        Logger::writeToLog ("Element v" + getApplicationVersion());
        Logger::writeToLog ("Copyright (c) 2017-2018 Kushview, LLC.  All rights reserved.\n");

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

        slaves.clearQuick (true);
                
        UnlockStatus& status (world->getUnlockStatus());
        status.save();
        
        auto engine (world->getAudioEngine());
        auto& plugins (world->getPluginManager());
        auto& settings (world->getSettings());
        auto* props = settings.getUserSettings();
        plugins.setPropertiesFile (nullptr); // must be done before Settings is deleted
        controller->deactivate();
        
        plugins.saveUserPlugins (settings);
        if (ScopedXml el = world->getDeviceManager().createStateXml())
            props->setValue ("devices", el);
        
        engine = nullptr;
        controller = nullptr;
        world->setEngine (nullptr);
        world->unloadModules();
        world = nullptr;
        
        // Analytics::getInstance()->deleteInstance();
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
            : AlertWindow::showYesNoCancelBox (AlertWindow::NoIcon,
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
        
        if (world->getSettings().scanForPluginsOnStartup())
            world->getPluginManager().scanAudioPlugins();
    
        controller = startup->controller.release();
        startup = nullptr;
        
        controller->run();

        if (world->getSettings().checkForUpdates())
            CurrentVersion::checkAfterDelay (12 * 1000, false);

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
    ScopedPointer<Globals>          world;
    ScopedPointer<AppController>    controller;
    ScopedPointer<Startup>          startup;
    OwnedArray<kv::ChildProcessSlave>   slaves;
    
    bool maybeLaunchSlave (const String& commandLine)
    {
        slaves.clearQuick (true);
        slaves.add (world->getPluginManager().createAudioPluginScannerSlave());
        StringArray processIds = { EL_PLUGIN_SCANNER_PROCESS_ID };
        for (auto* slave : slaves)
        {
            for (const auto& pid : processIds)
            {
                if (slave->initialiseFromCommandLine (commandLine, pid))
                {
				   #if JUCE_MAC
                    Process::setDockIconVisible (false);
				   #endif
                    juce::shutdownJuce_GUI();
                    return true;
                }
            }
        }
        
        return false;
    }
    
    void launchApplication()
    {
        if (nullptr != controller)
            return;
        
        startup = new Startup (*world, false, false);
        startup->addActionListener (this);
        startup->launchApplication();
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

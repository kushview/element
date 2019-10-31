/*
    This file is part of Element
    Copyright (C) 2016-2019  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#if ! EL_RUNNING_AS_PLUGIN

#include "ElementApp.h"
#include "controllers/AppController.h"
#include "controllers/GraphController.h"
#include "controllers/SessionController.h"
#include "engine/InternalFormat.h"
#include "engine/GraphProcessor.h"
#include "session/DeviceManager.h"
#include "session/PluginManager.h"
#include "Commands.h"
#include "DataPath.h"
#include "Globals.h"
#include "Messages.h"
#include "Version.h"
#include "Settings.h"
#include "Utils.h"

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
        if (auto dxml = props->getXmlValue ("devices"))
        {
            devices.initialise (DeviceManager::maxAudioChannels,
                                DeviceManager::maxAudioChannels, 
                                dxml.get(), true, "default", nullptr);
        }
        else
        {
            devices.initialiseWithDefaultDevices (DeviceManager::maxAudioChannels,
                                                  DeviceManager::maxAudioChannels);
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
        Settings& settings (world.getSettings());
        
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
        controller = new AppController (world);

        setupPlugins();
        setupKeyMappings();
        setupAudioEngine();
        setupMidiEngine();

        sendActionMessage ("finishedLaunching");
    }
    
    void setupAudioEngine()
    {

    }

    void setupMidiEngine()
    {
        auto& midi = world.getMidiEngine();
        midi.applySettings (world.getSettings());
    }

    void setupKeyMappings()
    {
        auto* const props = world.getSettings().getUserSettings();
        auto* const keymp = world.getCommandManager().getKeyMappings();
        if (props && keymp)
        {
            std::unique_ptr<XmlElement> xml;
            xml = props->getXmlValue ("keymappings");
            if (xml != nullptr)
                world.getCommandManager().getKeyMappings()->restoreFromXml (*xml);
            xml = nullptr;
        }
    }

    void setupPlugins()
    {
        auto& settings (world.getSettings());
        auto& plugins  (world.getPluginManager());
        auto  engine   (world.getAudioEngine());
        
        plugins.addDefaultFormats();
        plugins.addFormat (new InternalFormat (*engine, world.getMidiEngine()));
        plugins.addFormat (new ElementAudioPluginFormat (world));
        plugins.restoreUserPlugins (settings);
        plugins.setPropertiesFile (settings.getUserSettings());
        plugins.scanInternalPlugins();
        plugins.searchUnverifiedPlugins();

        DBG("[EL] supported plugin formats:");
        auto& audioPlugs = plugins.getAudioPluginFormats ();
        for (int i = 0; i < audioPlugs.getNumFormats(); ++i) {
            auto name = audioPlugs.getFormat (i)->getName();
            if (name == "Internal" || name == "Element")
                continue;
            DBG("[EL]   " << name);
        }
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

// uncomment to clear license on load
// #define EL_CLEAR_LICENSE    1

class Application : public JUCEApplication,
                    public ActionListener
{
public:
    Application() { }
    virtual ~Application() { }

    const String getApplicationName()    override      { return Util::appName(); }
    const String getApplicationVersion() override      { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed()    override      { return true; }

    void initialise (const String& commandLine ) override
    {
        world = new Globals (commandLine);
        if (maybeLaunchSlave (commandLine))
            return;
        
        if (sendCommandLineToPreexistingInstance())
        {
            quit();
            return;
        }

        loadLicense();
        initializeModulePath();
        printCopyNotice();
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
        
        auto engine (world->getAudioEngine());
        auto& plugins (world->getPluginManager());
        auto& settings (world->getSettings());
        auto& midi (world->getMidiEngine());
        auto* props = settings.getUserSettings();
        plugins.setPropertiesFile (nullptr); // must be done before Settings is deleted

        controller->saveSettings();
        controller->deactivate();
        
        plugins.saveUserPlugins (settings);
        midi.writeSettings (settings);
        
        if (auto el = world->getDeviceManager().createStateXml())
            props->setValue ("devices", el.get());
        if (auto keymappings = world->getCommandManager().getKeyMappings()->createXml (true))
            props->setValue ("keymappings", keymappings.get());

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
        
       #if defined (EL_PRO)
        auto* sc = controller->findChild<SessionController>();
        
        if (world->getSettings().askToSaveSession())
        {
            // - 0 if the third button was pressed ('cancel')
            // - 1 if the first button was pressed ('yes')
            // - 2 if the middle button was pressed ('no')

            const int res = !sc->hasSessionChanged() ? 2
                : AlertWindow::showYesNoCancelBox (AlertWindow::NoIcon, "Save Session",
                    "This session may have changes. Would you like to save before exiting?");
            if (res == 1)
                sc->saveSession();
            
            if (res != 0)
                Application::quit();
        }
        else
        {
            sc->saveSession();
            Application::quit();
        }

       #else // lite and solo
        auto* gc = controller->findChild<GraphController>();
        if (world->getSettings().askToSaveSession())
        {
            // - 0 if the third button was pressed ('cancel')
            // - 1 if the first button was pressed ('yes')
            // - 2 if the middle button was pressed ('no')
            const int res = ! gc->hasGraphChanged() ? 2
                : AlertWindow::showYesNoCancelBox (AlertWindow::NoIcon, "Save Graph",
                    "This graph may have changes. Would you like to save before exiting?");
            if (res == 1)
                gc->saveGraph (false);
            
            if (res != 0)
                Application::quit();
        }
        else
        {
            gc->saveGraph (false);
            Application::quit();
        }
       #endif
    }

    void anotherInstanceStarted (const String& commandLine) override
    {
        if (! controller)
            return;
        
       #if EL_PRO
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
       #endif
    }

    void resumed() override
    {
       #if JUCE_WINDOWS
        auto& devices (world->getDeviceManager());
        devices.restartLastAudioDevice();
       #endif
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

       #ifndef EL_FREE
        if (world->getSettings().checkForUpdates())
            CurrentVersion::checkAfterDelay (12 * 1000, false);
       #endif

       #if EL_PRO
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
       #endif
    }
    
private:
    String launchCommandLine;
    ScopedPointer<Globals>          world;
    ScopedPointer<AppController>    controller;
    ScopedPointer<Startup>          startup;
    OwnedArray<kv::ChildProcessSlave>   slaves;
    
    void loadLicense()
    {
    
    }

    void printCopyNotice()
    {
        String appName = Util::appName();
        appName << " v" << getApplicationVersion();
    #if 0
       #if defined (EL_PRO) || defined (EL_SOLO)
        if (EL_IS_NOT_ACTIVATED (world->getUnlockStatus()))
        {
            appName << " (unauthorized)";
        }
        else if (EL_IS_ACTIVATED (world->getUnlockStatus()))
        {
            appName << " (authorized)";
        }
        else if (EL_IS_TRIAL_EXPIRED (world->getUnlockStatus()))
        {
            appName << " (trial expired)";
        }
        else if (EL_IS_TRIAL_NOT_EXPIRED (world->getUnlockStatus()))
        {
            appName << " (trial)";
        }
        else
        {
            jassertfalse; // this is most likely a wrong product.  e.g. SE license used for Pro
            appName << " (unkown activation state)";
        }
       #else
            appName << " (lite)";
       #endif
       
        Logger::writeToLog (appName);
        Logger::writeToLog ("Copyright (c) 2017-2019 Kushview, LLC.  All rights reserved.\n");
       #if EL_USE_LOCAL_AUTH
        Logger::writeToLog ("[EL] using local authentication");
       #endif
       #endif
    }

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

#endif

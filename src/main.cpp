/*
    This file is part of Element
    Copyright (C) 2016-2021  Kushview, LLC.  All rights reserved.

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

#include "ElementApp.h"
#include "controllers/AppController.h"
#include "controllers/GraphController.h"
#include "controllers/SessionController.h"
#include "engine/internalformat.hpp"
#include "scripting/scriptingengine.hpp"
#include "session/devicemanager.hpp"
#include "session/pluginmanager.hpp"
#include "commands.hpp"
#include "datapath.hpp"
#include "globals.hpp"
#include "log.hpp"
#include "messages.hpp"
#include "version.hpp"
#include "settings.hpp"
#include "utils.hpp"

namespace Element {

class Startup : public ActionBroadcaster,
                private Thread
{
public:
    Startup (Globals& w, const bool useThread = false, const bool splash = false)
        : Thread ("ElementStartup"),
          world (w),
          usingThread (useThread),
          showSplash (splash),
          isFirstRun (false)
    {
    }

    ~Startup() {}

    void launchApplication()
    {
        Settings& settings (world.getSettings());
        isFirstRun = ! settings.getUserSettings()->getFile().existsAsFile();
        DataPath path;
        ignoreUnused (path);

        updateSettingsIfNeeded();

        

        if (usingThread)
        {
            startThread();
            while (isThreadRunning())
                MessageManager::getInstance()->runDispatchLoopUntil (30);
        }
        else
        {
            if (showSplash)
                (new StartupScreen())->deleteAfterDelay (RelativeTime::seconds (5), true);
            this->run();
        }
    }

    std::unique_ptr<AppController> controller;

    const bool isUsingThread() const { return usingThread; }

private:
    friend class Application;
    Globals& world;
    const bool usingThread;
    const bool showSplash;
    bool isFirstRun;

    class StartupScreen : public SplashScreen
    {
    public:
        StartupScreen()
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
        ignoreUnused (settings);
    }

    void run() override
    {
        setupLogging();
        setupKeyMappings();
        setupAudioEngine();
        setupPlugins();
        setupMidiEngine();
        setupScripting();

        sendActionMessage ("finishedLaunching");
    }

    void setupAudioEngine()
    {
        auto& settings = world.getSettings();
        DeviceManager& devices (world.getDeviceManager());
        String tp = devices.getCurrentAudioDeviceType();

        AudioEnginePtr engine = new AudioEngine (world);
        engine->applySettings (settings);
        world.setEngine (engine); // this will also instantiate the session
        controller = std::make_unique<AppController> (world);

        auto* props = settings.getUserSettings();
        
        if (auto dxml = props->getXmlValue ("devices"))
        {
            devices.initialise (DeviceManager::maxAudioChannels,
                                DeviceManager::maxAudioChannels,
                                dxml.get(),
                                true,
                                "default",
                                nullptr);
            auto setup = devices.getAudioDeviceSetup();
            devices.setAudioDeviceSetup (setup, true);
        }
        else
        {
            devices.initialiseWithDefaultDevices (DeviceManager::maxAudioChannels,
                                                  DeviceManager::maxAudioChannels);
        }
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
        auto& plugins (world.getPluginManager());
        auto engine (world.getAudioEngine());

        plugins.addDefaultFormats();
        plugins.addFormat (new InternalFormat (*engine, world.getMidiEngine()));
        plugins.addFormat (new ElementAudioPluginFormat (world));
        plugins.restoreUserPlugins (settings);
        plugins.setPropertiesFile (settings.getUserSettings());
        plugins.scanInternalPlugins();
        plugins.searchUnverifiedPlugins();
    }

    void setupScripting()
    {
        auto& scripts = world.getScriptingEngine();
        ignoreUnused (scripts);
    }

    void setupLogging()
    {
        Logger::setCurrentLogger (&world.getLog());
    }
};

class Application : public JUCEApplication,
                    public ActionListener
{
public:
    Application() {}
    virtual ~Application() {}

    const String getApplicationName() override { return Util::appName(); }
    const String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const String& commandLine) override
    {
        world = new Globals (commandLine);
        if (maybeLaunchSlave (commandLine))
            return;

        if (sendCommandLineToPreexistingInstance())
        {
            quit();
            return;
        }

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
        Logger::setCurrentLogger (nullptr);
        world->setEngine (nullptr);
        world = nullptr;
    }

    void systemRequestedQuit() override
    {
        if (! controller)
        {
            Application::quit();
            return;
        }

#ifndef EL_SOLO
        auto* sc = controller->findChild<SessionController>();

        if (world->getSettings().askToSaveSession())
        {
            // - 0 if the third button was pressed ('cancel')
            // - 1 if the first button was pressed ('yes')
            // - 2 if the middle button was pressed ('no')
            const int res = ! sc->hasSessionChanged() ? 2
                                                      : AlertWindow::showYesNoCancelBox (AlertWindow::NoIcon, "Save Session", "This session may have changes. Would you like to save before exiting?");

            if (res == 1)
                sc->saveSession();
            if (res != 0)
                Application::quit();
        }
        else
        {
            if (sc->getSessionFile().existsAsFile())
            {
                sc->saveSession (false, false, false);
            }
            else
            {
                if (AlertWindow::showOkCancelBox (AlertWindow::NoIcon, "Save Session", "This session has not been saved to disk yet.\nWould you like to before exiting?", "Yes", "No"))
                {
                    sc->saveSession();
                }
            }

            Application::quit();
        }

#else // SE
        auto* gc = controller->findChild<GraphController>();
        if (world->getSettings().askToSaveSession())
        {
            // - 0 if the third button was pressed ('cancel')
            // - 1 if the first button was pressed ('yes')
            // - 2 if the middle button was pressed ('no')
            const int res = ! gc->hasGraphChanged() ? 2
                                                    : AlertWindow::showYesNoCancelBox (AlertWindow::NoIcon, "Save Graph", "This graph may have changes. Would you like to save before exiting?");
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

#ifndef EL_SOLO
        if (auto* sc = controller->findChild<SessionController>())
        {
            const auto path = commandLine.unquoted().trim();
            if (File::isAbsolutePath (path))
            {
                const File file (path);
                if (file.hasFileExtension ("els"))
                    sc->openFile (file);
                else if (file.hasFileExtension ("elg"))
                    sc->importGraph (file);
            }
        }
#else
        if (auto* gc = controller->findChild<GraphController>())
        {
            const auto path = commandLine.unquoted().trim();
            if (File::isAbsolutePath (path))
            {
                const File file (path);
                if (file.hasFileExtension ("elg"))
                    gc->openGraph (file);
            }
        }
#endif
    }

    void suspended() override {}

    void resumed() override
    {
        auto& devices (world->getDeviceManager());
        devices.restartLastAudioDevice();
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

#ifndef EL_SOLO
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
    ScopedPointer<Globals> world;
    ScopedPointer<AppController> controller;
    ScopedPointer<Startup> startup;
    OwnedArray<kv::ChildProcessSlave> slaves;

    void printCopyNotice()
    {
        String appName = Util::appName();
        appName << " v" << getApplicationVersion() << " (GPL v3)";
        Logger::writeToLog (appName);
        Logger::writeToLog (String ("Copyright (c) 2017-%YEAR% Kushview, LLC.  All rights reserved.\n")
                                .replace ("%YEAR%", String (Time::getCurrentTime().getYear())));
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
        File modDir = path.getParentDirectory().getParentDirectory().getChildFile ("lib/element").getFullPathName();
#if JUCE_DEBUG
        if (! modDir.exists())
        {
            modDir = path.getParentDirectory().getParentDirectory().getChildFile ("modules");
        }
#endif

#if JUCE_WINDOWS
        String putEnv = "ELEMENT_MODULE_PATH=";
        putEnv << modDir.getFullPathName();
        putenv (putEnv.toRawUTF8());
#else
        setenv ("ELEMENT_MODULE_PATH", modDir.getFullPathName().toRawUTF8(), 1);
#endif
    }
};

} // namespace Element

START_JUCE_APPLICATION (Element::Application)

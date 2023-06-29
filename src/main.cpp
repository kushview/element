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
#include <element/services.hpp>
#include <element/version.hpp>
#include "engine/internalformat.hpp"

#include <element/context.hpp>
#include <element/devices.hpp>
#include <element/plugins.hpp>
#include <element/settings.hpp>
#include <element/ui.hpp>

#include <element/ui/commands.hpp>
#include "engine/midiengine.hpp"
#include "scripting.hpp"
#include <element/ui/commands.hpp>
#include "datapath.hpp"
#include "services/sessionservice.hpp"
#include "log.hpp"
#include "messages.hpp"
#include "utils.hpp"
#include "slaveprocess.hpp"

namespace element {

class Startup : public ActionBroadcaster
{
public:
    Startup (Context& w)
        : world (w), isFirstRun (false) {}
    ~Startup() {}

    void launchApplication()
    {
        Settings& settings (world.settings());
        isFirstRun = ! settings.getUserSettings()->getFile().existsAsFile();

        setupLogging();
        setupKeyMappings();
        setupAudioEngine();
        setupPlugins();
        setupMidiEngine();
        setupScripting();

        sendActionMessage ("finishedLaunching");
    }

private:
    friend class Application;
    Context& world;
    bool isFirstRun;

    void setupAudioEngine()
    {
        auto& settings = world.settings();
        DeviceManager& devices (world.devices());
        String tp = devices.getCurrentAudioDeviceType();

        AudioEnginePtr engine = world.audio();
        engine->applySettings (settings);

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
        auto& midi = world.midi();
        midi.applySettings (world.settings());
    }

    void setupKeyMappings()
    {
        auto* const props = world.settings().getUserSettings();
        auto* const keymp = world.services().find<GuiService>()->commands().getKeyMappings();
        if (props && keymp)
        {
            std::unique_ptr<XmlElement> xml;
            xml = props->getXmlValue (Settings::keymappingsKey);
            if (xml != nullptr)
                keymp->restoreFromXml (*xml);
            xml = nullptr;
        }
    }

    void setupPlugins()
    {
        auto& settings (world.settings());
        auto& plugins (world.plugins());
        auto engine (world.audio());
        plugins.restoreUserPlugins (settings);
        plugins.setPropertiesFile (settings.getUserSettings());
        plugins.scanInternalPlugins();
        plugins.searchUnverifiedPlugins();
    }

    void setupScripting()
    {
        auto& scripts = world.scripting();
        ignoreUnused (scripts);
    }

    void setupLogging()
    {
        Logger::setCurrentLogger (&world.logger());
    }
};

class Application : public JUCEApplication,
                    public ActionListener,
                    private Timer
{
public:
    Application() {}
    virtual ~Application() {}

    const String getApplicationName() override { return "Element"; }
    const String getApplicationVersion() override { return EL_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const String& commandLine) override
    {
        world = std::make_unique<Context> (commandLine);
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
        if (! world)
            return;

        slaves.clearQuick (true);

        auto engine (world->audio());
        auto& plugins (world->plugins());
        auto& settings (world->settings());
        auto& midi (world->midi());
        auto* props = settings.getUserSettings();
        plugins.setPropertiesFile (nullptr); // must be done before Settings is deleted

        auto& srvs = world->services();
        srvs.saveSettings();
        srvs.deactivate();
        srvs.shutdown();

        plugins.saveUserPlugins (settings);
        midi.writeSettings (settings);

        if (auto el = world->devices().createStateXml())
            props->setValue (Settings::devicesKey, el.get());
        auto& ui = *world->services().find<UI>();
        if (auto keymappings = ui.commands().getKeyMappings()->createXml (true))
            props->setValue (Settings::keymappingsKey, keymappings.get());

        engine = nullptr;
        Logger::setCurrentLogger (nullptr);
        world->setEngine (nullptr);
        world = nullptr;
    }

    void systemRequestedQuit() override
    {
        if (! world)
        {
            Application::quit();
            return;
        }

        auto* sc = world->services().find<SessionService>();

        if (world->settings().askToSaveSession())
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
    }

    void anotherInstanceStarted (const String& commandLine) override
    {
        if (! world)
            return;

        if (auto* sc = world->services().find<SessionService>())
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
    }

    void suspended() override {}

    void resumed() override
    {
        auto& devices (world->devices());
        devices.restartLastAudioDevice();
    }

    void finishLaunching()
    {
        if (nullptr == startup)
            return;

        if (world->settings().scanForPluginsOnStartup())
            world->plugins().scanAudioPlugins();

        startup.reset();

        world->services().run();

        if (world->settings().checkForUpdates())
            startTimer (5000);

        const auto path = getCommandLineParameters();
        const File sessionFile = File::isAbsolutePath (path) ? File (path)
                                                             : File::getCurrentWorkingDirectory().getChildFile (path);
        if (sessionFile.hasFileExtension ("els"))
            if (auto* sc = world->services().find<SessionService>())
                sc->openFile (sessionFile);
    }

private:
    String launchCommandLine;
    std::unique_ptr<Context> world;
    std::unique_ptr<Startup> startup;
    OwnedArray<element::ChildProcessSlave> slaves;

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
        slaves.add (world->plugins().createAudioPluginScannerSlave());
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
        if (startup != nullptr)
            return;

        startup = std::make_unique<Startup> (*world);
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

    void timerCallback() override
    {
        std::clog << "[element] checking updates..." << std::endl;
        world->services().find<UI>()->checkUpdates();
        stopTimer();
    }
};

} // namespace element

START_JUCE_APPLICATION (element::Application)

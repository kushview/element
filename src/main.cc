// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "ElementApp.h"
#include <element/services.hpp>
#include <element/version.hpp>
#include <element/context.hpp>
#include <element/devices.hpp>
#include <element/plugins.hpp>
#include <element/settings.hpp>
#include <element/ui.hpp>
#include <element/ui/commands.hpp>
#include <element/ui/commands.hpp>
#include <element/ui/updater.hpp>

#include "engine/internalformat.hpp"
#include "engine/midiengine.hpp"
#include "scripting.hpp"
#include "datapath.hpp"
#include "services/sessionservice.hpp"
#include "log.hpp"
#include "messages.hpp"
#include "utils.hpp"

#ifndef EL_FIRST_RUN
#define EL_FIRST_RUN 0
#endif

namespace element {

class Startup : public ActionBroadcaster
{
public:
    Startup (Context& w)
        : world (w), isFirstRun (false) {}
    ~Startup() {}

    void launchApplication()
    {
        DataPath::initializeDefaultLocation();

        [[maybe_unused]] Settings& settings (world.settings());
#if EL_FIRST_RUN
        isFirstRun = true;
#else
        isFirstRun = ! settings.getUserSettings()->getFile().existsAsFile();
#endif
        setupLogging();
        setupKeyMappings();
        setupAudioEngine();
        setupPlugins();
        setupMidiEngine();
        setupScripting();
        setupRepos();

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
        for (const auto& tp : devices.getAvailableDeviceTypes())
            tp->scanForDevices();

        AudioEnginePtr engine = world.audio();
        engine->applySettings (settings);

        auto* props = settings.getUserSettings();

        String error = "No device found at startup";
        if (auto dxml = props->getXmlValue ("devices"))
        {
            error = devices.initialise (DeviceManager::maxAudioChannels,
                                        DeviceManager::maxAudioChannels,
                                        dxml.get(),
                                        true,
                                        "*",
                                        nullptr);
            if (error.isNotEmpty())
            {
                auto setup = devices.getAudioDeviceSetup();
                error = devices.setAudioDeviceSetup (setup, true);
            }
        }

        if (error.isNotEmpty())
        {
#if JUCE_WINDOWS
            devices.setCurrentAudioDeviceType ("Windows Audio (Low Latency Mode)", true);
#endif
            error = devices.initialiseWithDefaultDevices (DeviceManager::maxAudioChannels,
                                                          DeviceManager::maxAudioChannels);
        }

        if (error.isNotEmpty())
        {
            Logger::writeToLog (error);
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
        plugins.restoreUserPlugins (settings);
        plugins.setPropertiesFile (settings.getUserSettings());

        if (isFirstRun)
        {
            auto props = settings.getUserSettings();
            for (const auto& formatName : Util::compiledAudioPluginFormats())
                props->setValue (Settings::lastPluginScanPathPrefix + formatName,
                                 plugins.defaultSearchPath (formatName).toString());
        }

        plugins.scanInternalPlugins();
        plugins.searchUnverifiedPlugins();
    }

    void setupScripting()
    {
        auto& scripts = world.scripting();
        ignoreUnused (scripts);
    }

    void setupRepos()
    {
        if (! isFirstRun)
            return;
#if EL_UPDATER
        auto& settings = world.settings();
        const auto repos (ui::Updater::repositories());
        if (repos.empty())
            return;

        const auto repo (repos.front());
        if (! juce::URL::isProbablyAWebsiteURL (repo.host))
            return;

        auto setPublic = [&]() -> void {
            settings.setUpdateChannel ("public");
            settings.setUpdateKeyUser ("");
            settings.setUpdateKey ("");
        };

        const juce::String host (repo.host);
        if (host.contains ("/public"))
        {
            setPublic();
            return;
        }

        const auto channel = [&]() -> juce::String {
            if (host.contains ("stable") || host.contains ("release"))
            {
                return "stable";
            }
            else if (host.contains ("nightly"))
            {
                return "nightly";
            }
            return "";
        }();

        const juce::String username (repo.username);
        const juce::String passData (repo.password);
        const auto keyType = passData.upToFirstOccurrenceOf (":", false, false);
        const auto key = passData.fromFirstOccurrenceOf (":", false, false);

        // clang-format off
        auto repoValid = [&]() -> bool {
            return username.isNotEmpty() &&
                key.isNotEmpty() &&
                (keyType == "member" || keyType == "element-v1" || keyType == "patreon") &&
                (channel == "stable" || channel == "nightly");
        };
        // clang-format on

        if (repoValid())
        {
            settings.setUpdateChannel (channel);
            settings.setUpdateKeyUser (username);
            settings.setUpdateKeyType (keyType);
            settings.setUpdateKey (key);
        }
        else
        {
            setPublic();
        }
#endif
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
    const String getApplicationVersion() override { return ELEMENT_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const String& commandLine) override
    {
        world = std::make_unique<Context> (RunMode::Standalone, commandLine);
        if (maybeLaunchScannerWorker (commandLine))
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

        workers.clearQuick (true);
        auto& srvs = world->services();
        srvs.saveSettings();

        auto& devices (world->devices());
        devices.closeAudioDevice();

        auto engine (world->audio());
        auto& plugins (world->plugins());
        auto& settings (world->settings());
        auto& midi (world->midi());
        auto* props = settings.getUserSettings();
        plugins.setPropertiesFile (nullptr); // must be done before Settings is deleted

        srvs.deactivate();
        srvs.shutdown();

        plugins.saveUserPlugins (settings);
        midi.writeSettings (settings);

        if (auto el = world->devices().createStateXml())
            props->setValue (Settings::devicesKey, el.get());

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

    void maybeOpenCommandLineFile (const String& commandLine)
    {
        if (auto* sc = world->services().find<SessionService>())
        {
            const auto path = commandLine.unquoted().trim();
            const File sessionFile = File::isAbsolutePath (path)
                                         ? File (path)
                                         : File::getCurrentWorkingDirectory().getChildFile (path);
            if (sessionFile.existsAsFile())
            {
                const File file (path);
                if (file.hasFileExtension ("els"))
                    sc->openFile (file);
                else if (file.hasFileExtension ("elg"))
                    sc->importGraph (file);
            }
        }
    }

    void anotherInstanceStarted (const String& commandLine) override
    {
        if (! world)
            return;

        maybeOpenCommandLineFile (commandLine);
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

        maybeOpenCommandLineFile (getCommandLineParameters());
    }

private:
    String launchCommandLine;
    std::unique_ptr<Context> world;
    std::unique_ptr<Startup> startup;
    OwnedArray<juce::ChildProcessWorker> workers;

    void printCopyNotice()
    {
        String appName = Util::appName();
        appName << " v" << getApplicationVersion() << " (GPL v3)";
        Logger::writeToLog (appName);
        Logger::writeToLog (String ("Copyright (c) 2017-%YEAR% Kushview, LLC.  All rights reserved.\n")
                                .replace ("%YEAR%", String (Time::getCurrentTime().getYear())));
    }

    bool maybeLaunchScannerWorker (const String& commandLine)
    {
        workers.clearQuick (true);
        workers.add (world->plugins().createAudioPluginScannerWorker());
        StringArray processIds = { EL_PLUGIN_SCANNER_PROCESS_ID };
        for (auto* worker : workers)
        {
            for (const auto& pid : processIds)
            {
                if (worker->initialiseFromCommandLine (commandLine, pid, 20 * 1000))
                {
#if JUCE_MAC
                    Process::setDockIconVisible (false);
#endif
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
        world->logger().logMessage ("[element] checking updates...");
        world->services().find<UI>()->checkUpdates();
        stopTimer();
    }
};

} // namespace element

START_JUCE_APPLICATION (element::Application)

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/nodefactory.hpp>
#include <element/node.hpp>
#include <element/plugins.hpp>
#include <element/settings.hpp>
#include <element/lv2.hpp>

#include "nodes/nodetypes.hpp"
#include "engine/ionode.hpp"
#include "datapath.hpp"
#include "utils.hpp"

#define EL_DEAD_AUDIO_PLUGINS_FILENAME "scanner/crashed.txt"
#define EL_PLUGIN_SCANNER_SLAVE_LIST_PATH "scanner/list.xml"
#define EL_PLUGIN_SCANNER_WAITING_STATE "waiting"
#define EL_PLUGIN_SCANNER_READY_STATE "ready"

#define EL_PLUGIN_SCANNER_READY_ID "ready"
#define EL_PLUGIN_SCANNER_START_ID "start"
#define EL_PLUGIN_SCANNER_FINISHED_ID "finished"

#define EL_PLUGIN_SCANNER_DEFAULT_TIMEOUT 20000 // 20 Seconds

namespace element {
using namespace juce;

static const char* pluginListKey() { return Settings::pluginListKey; }
/* noop. prevent OS error dialogs from child process */
static void pluginScannerWorkerCrashHandler (void*) {}

//==============================================================================
class PluginScannerCoordinator : public juce::ChildProcessCoordinator,
                                 public AsyncUpdater
{
public:
    explicit PluginScannerCoordinator (PluginScanner& o) : owner (o) {}
    ~PluginScannerCoordinator() {}

    bool startScanning (const StringArray& names = StringArray())
    {
        if (isRunning())
            return true;

        {
            ScopedLock sl (lock);
            slaveState = EL_PLUGIN_SCANNER_WAITING_STATE;
            running = false;
            formatNames = names;
        }

        const bool res = launchScanner();

        {
            ScopedLock sl (lock);
            running = res;
        }

        DBG ("[element] scanner launched: " << (isRunning() ? "yes" : "no"));
        return res;
    }

    void handleMessageFromWorker (const MemoryBlock& mb) override
    {
        const auto data (mb.toString());
        const auto type (data.upToFirstOccurrenceOf (":", false, false));
        const auto message (data.fromFirstOccurrenceOf (":", false, false));

        if (type == "state")
        {
            ScopedLock sl (lock);
            const String lastState = slaveState;
            slaveState = message;
            if (lastState != slaveState)
            {
                ScopedUnlock sul (lock);
                triggerAsyncUpdate();
            }
        }
        else if (type == "name")
        {
            owner.listeners.call (&PluginScanner::Listener::audioPluginScanStarted, message.trim());
            ScopedLock sl (lock);
            pluginBeingScanned = message.trim();
        }
        else if (type == "progress")
        {
            float newProgress = (float) var (message);
            owner.listeners.call (&PluginScanner::Listener::audioPluginScanProgress, newProgress);
            ScopedLock sl (lock);
            progress = newProgress;
        }
    }

    void handleConnectionLost() override
    {
        // this probably will happen when a plugin crashes.
        {
            std::clog << "[element] scanner connection lost...\n";
            ScopedLock sl (lock);
            running = false;
        }

        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        const auto state = getWorkerState();
        if (state == "ready" && isRunning())
        {
            String msg = "scan:";
            msg << formatNames.joinIntoString (",");
            MemoryBlock mb (msg.toRawUTF8(), msg.length());
            sendMessageToWorker (mb);
        }
        else if (state == "scanning")
        {
            if (! isRunning())
            {
                DBG ("[element] a plugin crashed or timed out during scan");
                updateListAndLaunchWorker();
            }
            else
            {
                DBG ("[element] scanning... and running....");
            }
        }
        else if (state == "finished")
        {
            DBG ("[element] worker finished scanning");
            {
                ScopedLock sl (lock);
                running = false;
                slaveState = "idle";
            }
            owner.listeners.call (&PluginScanner::Listener::audioPluginScanFinished);
        }
        else if (state == EL_PLUGIN_SCANNER_WAITING_STATE)
        {
            if (! isRunning())
            {
                DBG ("[element] waiting for plugin scanner");
                updateListAndLaunchWorker();
            }
        }
        else if (slaveState == "quitting")
        {
            return;
        }
        else
        {
            DBG ("[element] invalid worker state: " << state);
        }
    }

    const String getWorkerState() const
    {
        ScopedLock sl (lock);
        return slaveState;
    }

    float getProgress() const
    {
        ScopedLock sl (lock);
        return progress;
    }

    bool isRunning() const
    {
        ScopedLock sl (lock);
        return running;
    }

    bool sendQuitMessage()
    {
        if (isRunning())
            return sendMessageToWorker (MemoryBlock ("quit", 4));
        return false;
    }

private:
    PluginScanner& owner;

    CriticalSection lock;
    bool running = false;
    float progress = 0.f;
    String slaveState;
    StringArray formatNames;
    StringArray faileFiles;

    String pluginBeingScanned;

    void updateListAndLaunchWorker()
    {
        if (auto xml = XmlDocument::parse (PluginScanner::getWorkerPluginListFile()))
            owner.list.recreateFromXml (*xml);

        const bool res = launchScanner();
        ScopedLock sl (lock);
        running = res;
    }

    void resetScannerVariables()
    {
        ScopedLock sl (lock);
        pluginBeingScanned = String();
        progress = -1.f;
    }

    bool launchScanner (const int timeout = EL_PLUGIN_SCANNER_DEFAULT_TIMEOUT, const int flags = 3)
    {
        resetScannerVariables();
        auto scannerExe = File::getSpecialLocation (File::currentExecutableFile);
        return launchWorkerProcess (scannerExe,
                                    EL_PLUGIN_SCANNER_PROCESS_ID,
                                    timeout,
                                    flags);
    }
};

//==============================================================================
class PluginScannerWorker : public juce::ChildProcessWorker,
                            public AsyncUpdater
{
public:
    PluginScannerWorker()
    {
        scanFile = PluginScanner::getWorkerPluginListFile();
        SystemStats::setApplicationCrashHandler (pluginScannerWorkerCrashHandler);
        auto logfile = DataPath::applicationDataDir().getChildFile ("log/scanner.log");
        logfile.create();
        logger = std::make_unique<juce::FileLogger> (logfile, "Plugin Scanner");
    }

    ~PluginScannerWorker()
    {
    }

    void handleMessageFromCoordinator (const MemoryBlock& mb) override
    {
        const auto data (mb.toString());
        const auto type (data.upToFirstOccurrenceOf (":", false, false));
        const auto message (data.fromFirstOccurrenceOf (":", false, false));

        String msg = "[scanner] received message: ";
        msg << type;
        logger->logMessage (msg);

        if (type == "quit")
        {
            handleConnectionLost();
            return;
        }

        if (type == "scan")
        {
            const auto formats (StringArray::fromTokens (message.trim(), ",", "'"));
            formatsToScan = formats;
            triggerAsyncUpdate();
        }
    }

    void handleAsyncUpdate() override
    {
        if (! scanFile.existsAsFile())
        {
            sendState ("scanning");
            sendState ("finished");
            return;
        }

        updateScanFileWithSettings();

        sendState ("scanning");

        for (const auto& format : formatsToScan)
            scanFor (format);

        settings->saveIfNeeded();
        sendState ("finished");

#if JUCE_LINUX
        // workaround to get the background process to quit
        handleConnectionLost();
#endif
    }

    void updateScanFileWithSettings()
    {
        if (! plugins)
            return;

        auto& list = plugins->getKnownPlugins();
        const auto types = list.getTypes();
        for (const auto& type : types)
            pluginList.addType (type);

        for (const auto& file : list.getBlacklistedFiles())
            pluginList.addToBlacklist (file);

        writePluginListNow();
    }

    void handleConnectionMade() override
    {
        logger->logMessage ("[scanner] connection to coordinator established");
        settings = std::make_unique<Settings>();
        plugins = std::make_unique<PluginManager>();
        logger->logMessage ("[scanner] created global objects");

        if (! scanFile.existsAsFile())
            scanFile.create();

        if (auto xml = XmlDocument::parse (scanFile))
            pluginList.recreateFromXml (*xml);

        logger->logMessage ("[scanner] processing blacklist");
        // This must happen before user settings, PluginManager will delete the deadman file
        // when restoring user plugins
        PluginDirectoryScanner::applyBlacklistingsFromDeadMansPedal (
            pluginList, plugins->getDeadAudioPluginsFile());

        logger->logMessage ("[scanner] setting up formats");
        plugins->addDefaultFormats();
        logger->logMessage ("[scanner] restoring plugin list");
        {
            juce::MessageManagerLock mml;
            plugins->restoreUserPlugins (*settings);
        }

        logger->logMessage ("[scanner] scanner notify read!");
        if (! sendState (EL_PLUGIN_SCANNER_READY_ID))
            logger->logMessage ("[scanner] plugin scanner failed to send 'ready' message");
    }

    void handleConnectionLost() override
    {
        logger.reset();
        settings = nullptr;
        plugins = nullptr;
        scanner = nullptr;
        exit (0);
    }

private:
    std::unique_ptr<Settings> settings;
    std::unique_ptr<PluginManager> plugins;
    std::unique_ptr<PluginDirectoryScanner> scanner;
    String fileOrIdentifier;
    KnownPluginList pluginList;
    StringArray filesToSkip;
    File scanFile;
    StringArray formatsToScan;

    std::unique_ptr<juce::FileLogger> logger;

    void applyDeadPlugins()
    {
        PluginDirectoryScanner::applyBlacklistingsFromDeadMansPedal (
            pluginList, plugins->getDeadAudioPluginsFile());
    }

    bool writePluginListNow()
    {
        applyDeadPlugins();
        if (auto xml = pluginList.createXml())
        {
            return xml->writeTo (scanFile);
        }
        return false;
    }

    bool sendState (const String& state)
    {
        return sendString ("state", state);
    }

    bool sendString (const String& type, const String& message)
    {
        String data = type;
        data << ":" << message.trim();
        MemoryBlock mb (data.toRawUTF8(), data.getNumBytesAsUTF8());
        return sendMessageToCoordinator (mb);
    }

    bool doNextScan()
    {
        const auto nextFile = scanner->getNextPluginFileThatWillBeScanned();
        sendString ("name", nextFile);
        logger->logMessage (String ("[scanner] scan: ") + nextFile);

        for (const auto& file : scanner->getFailedFiles())
            pluginList.addToBlacklist (file);

        if (scanner->scanNextFile (true, fileOrIdentifier))
        {
            writePluginListNow();
            return true;
        }

        return false;
    }

    void scanFor (const String& formatName)
    {
        if (plugins == nullptr || settings == nullptr)
            return;

        if (formatName == "LV2")
            scanLV2();
        else if (auto* format = plugins->getAudioPluginFormat (formatName))
            scanFor (*format);
    }

    void scanLV2()
    {
        if (plugins == nullptr || settings == nullptr)
            return;
        auto& nodes = plugins->getNodeFactory();

        for (auto* p : nodes.providers())
        {
            if (p->format() != "LV2")
                continue;
            const auto types = p->findTypes();
            float step = 1.f;
            for (const auto& tp : types)
            {
                if (! pluginList.getBlacklistedFiles().contains (tp) && pluginList.getTypeForFile (tp) == nullptr)
                {
                    sendString ("name", tp.trim());
                    logger->logMessage (String ("[scanner] scan: ") + tp);
                    OwnedArray<PluginDescription> dc;
                    pluginList.addToBlacklist (tp);
                    writePluginListNow();

                    if (auto inst = p->create (tp))
                    {
                        pluginList.removeFromBlacklist (tp);
                        PluginDescription desc;
                        inst->getPluginDescription (desc);
                        pluginList.addType (desc);
                        writePluginListNow();
                    }
                }

                sendString ("progress", String (step / (float) types.size()));
                step += 1.f;
            }
        }
    }

    void scanFor (AudioPluginFormat& format)
    {
        if (plugins == nullptr || settings == nullptr)
            return;

        const auto key = String (settings->lastPluginScanPathPrefix) + format.getName();
        FileSearchPath path (settings->getUserSettings()->getValue (key));
        scanner = std::make_unique<PluginDirectoryScanner> (pluginList, format, path, true, plugins->getDeadAudioPluginsFile(), false);

        while (doNextScan())
            sendString ("progress", String (scanner->getProgress()));

        writePluginListNow();
#if JUCE_LINUX
        Thread::sleep (1000);
#endif
    }
};

//==============================================================================
PluginScanner::PluginScanner (KnownPluginList& listToManage) : list (listToManage) {}
PluginScanner::~PluginScanner()
{
    listeners.clear();
    master.reset();
}

void PluginScanner::cancel()
{
    if (master)
    {
        master->cancelPendingUpdate();
        master->sendQuitMessage();
        master.reset();
    }
}

bool PluginScanner::isScanning() const { return master && master->isRunning(); }

void PluginScanner::scanForAudioPlugins (const juce::String& formatName)
{
    scanForAudioPlugins (StringArray ({ formatName }));
}

void PluginScanner::scanForAudioPlugins (const StringArray& formats)
{
    cancel();
    getWorkerPluginListFile().deleteFile();
    if (master == nullptr)
        master.reset (new PluginScannerCoordinator (*this));
    if (master->isRunning())
        return;
    master->startScanning (formats);
}

void PluginScanner::timerCallback()
{
}

// MARK: Unverified Plugins

typedef HashMap<String, StringArray> UnverifiedPluginMap;
typedef HashMap<String, FileSearchPath> UnverifiedPluginPaths;

class UnverifiedPlugins : private Thread
{
public:
    UnverifiedPlugins() : Thread ("euvpl") {}

    ~UnverifiedPlugins()
    {
        cancelFlag.set (1);
        if (isThreadRunning())
            stopThread (1000);
    }

    void searchForPlugins (PropertiesFile* props)
    {
        if (isThreadRunning())
            return;

        if (props)
        {
            for (const auto& f : Util::getSupportedAudioPluginFormats())
            {
                const auto key = String (Settings::lastPluginScanPathPrefix) + f;
                paths.set (f, FileSearchPath (props->getValue (key)));
            }
        }
        else
        {
            paths.clear();
        }

        startThread (Thread::Priority::background);
    }

    void getPlugins (OwnedArray<PluginDescription>& plugs,
                     const String& format,
                     KnownPluginList& list)
    {
        ScopedLock sl (lock);
        if (plugins.contains (format))
        {
            for (const auto& file : plugins.getReference (format))
            {
                if (nullptr != list.getTypeForFile (file))
                    continue;
                auto* const desc = plugs.add (new PluginDescription());
                desc->pluginFormatName = format;
                desc->fileOrIdentifier = file;
            }
        }
    }

private:
    friend class Thread;
    CriticalSection lock;
    UnverifiedPluginMap plugins;
    UnverifiedPluginPaths paths;
    Atomic<int> cancelFlag;

    void run() override
    {
        cancelFlag.set (0);

        PluginManager pluginManager;
        pluginManager.addDefaultFormats();
        auto& manager (pluginManager.getAudioPluginFormats());

        for (int i = 0; i < manager.getNumFormats(); ++i)
        {
            if (threadShouldExit() || cancelFlag.get() != 0)
                break;

            auto* const format = manager.getFormat (i);
            FileSearchPath path = paths[format->getName()];
            path.addPath (format->getDefaultLocationsToSearch());
            const auto found = format->searchPathsForPlugins (path, true, false);

            ScopedLock sl (lock);
            plugins.set (format->getName(), found);
        }

        cancelFlag.set (0);
    }
};

// MARK: Plugin Manager

class PluginManager::Private : public PluginScanner::Listener
{
public:
    Private (PluginManager& o)
        : owner (o)
    {
        deadAudioPlugins = DataPath::applicationDataDir().getChildFile (EL_DEAD_AUDIO_PLUGINS_FILENAME);
    }

    ~Private() {}

    /** returns true if anything changed in the plugin list */
    bool updateBlacklistedAudioPlugins()
    {
        bool didSomething = false;

        if (deadAudioPlugins.existsAsFile())
        {
            PluginDirectoryScanner::applyBlacklistingsFromDeadMansPedal (
                allPlugins, deadAudioPlugins);
            deadAudioPlugins.deleteFile();
            didSomething = true;
        }

        return didSomething;
    }

    void searchUnverifiedPlugins (PropertiesFile* props)
    {
        unverified.searchForPlugins (props);
    }

    void getUnverifiedPlugins (const String& format, OwnedArray<PluginDescription>& plugs)
    {
        unverified.getPlugins (plugs, format, allPlugins);
    }

private:
    friend class PluginManager;
    PluginManager& owner;
    AudioPluginFormatManager formats;
    KnownPluginList allPlugins;
    File deadAudioPlugins;
    UnverifiedPlugins unverified;
    NodeFactory nodes;
    double sampleRate = 44100.0;
    int blockSize = 512;
    std::unique_ptr<PluginScanner> scanner;
    bool hasAddedFormats = false;

    void scanAudioPlugins (const StringArray& names)
    {
        if (scanner)
        {
            scanner->removeListener (this);
            scanner->cancel();
            scanner = nullptr;
            std::clog << "[element] cancel already running scanner.\n";
        }

        StringArray formatsToScan = names;
        if (formatsToScan.isEmpty())
        {
            for (int i = 0; i < formats.getNumFormats(); ++i)
                if (formats.getFormat (i)->getName() != "Element" && formats.getFormat (i)->canScanForPlugins())
                    formatsToScan.add (formats.getFormat (i)->getName());
            formatsToScan.add ("LV2");
        }

        std::clog << "[element] preparing to scan: ";
        for (const auto& fmt : formatsToScan)
            std::clog << fmt.toStdString() << " ";
        std::clog << std::endl;

        scanner = std::make_unique<PluginScanner> (allPlugins);
        scanner->addListener (this);
        scanner->scanForAudioPlugins (formatsToScan);
    }

    void audioPluginScanFinished() override
    {
        {
            ScopedLock sl (lock);
            scannedPlugin = String();
            progress = -1.0;
        }

        owner.scanFinished();
    }

    void audioPluginScanStarted (const String& plugin) override
    {
        DBG ("[element] scanning: " << plugin);
        ScopedLock sl (lock);
        scannedPlugin = plugin;
    }

    void audioPluginScanProgress (const float p) override
    {
        ScopedLock sl (lock);
        progress = p;
    }

    String getScannedPluginName() const
    {
        ScopedLock sl (lock);
        return scannedPlugin;
    }

private:
    CriticalSection lock;
    String scannedPlugin;
    float progress = -1.0;
};

PluginManager::PluginManager()
{
    priv.reset (new Private (*this));
}

PluginManager::~PluginManager()
{
    priv.reset();
}

void PluginManager::addDefaultFormats()
{
    if (priv->hasAddedFormats)
        return;

    priv->nodes.add (new LV2NodeProvider());
    auto& audioPlugs = getAudioPluginFormats();
    for (const auto& fmt : Util::getSupportedAudioPluginFormats())
    {
        if (fmt == "")
            continue;

#if JUCE_MAC && JUCE_PLUGINHOST_AU
        else if (fmt == "AudioUnit")
            audioPlugs.addFormat (new AudioUnitPluginFormat());
#endif

#if JUCE_PLUGINHOST_VST
        else if (fmt == "VST")
            audioPlugs.addFormat (new VSTPluginFormat());
#endif

#if JUCE_PLUGINHOST_VST3
        else if (fmt == "VST3")
            audioPlugs.addFormat (new VST3PluginFormat());
#endif

#if JUCE_PLUGINHOST_LADSPA
        else if (fmt == "LADSPA")
            audioPlugs.addFormat (new LADSPAPluginFormat());
#endif
    }

    priv->hasAddedFormats = true;
}

void PluginManager::addFormat (AudioPluginFormat* fmt)
{
    getAudioPluginFormats().addFormat (fmt);
}

NodeFactory& PluginManager::getNodeFactory() { return priv->nodes; }

void PluginManager::addToKnownPlugins (const PluginDescription& desc)
{
    auto* const format = getAudioPluginFormat (desc.pluginFormatName);
    auto& list = priv->allPlugins;
    if (format && nullptr == list.getTypeForFile (desc.fileOrIdentifier))
    {
        OwnedArray<PluginDescription> dummy;
        list.removeFromBlacklist (desc.fileOrIdentifier);
        list.scanAndAddFile (desc.fileOrIdentifier, true, dummy, *format);
    }
}

void PluginManager::searchUnverifiedPlugins()
{
    if (! priv)
        return;
    priv->searchUnverifiedPlugins (this->props);
}

juce::ChildProcessWorker* PluginManager::createAudioPluginScannerWorker()
{
    return new PluginScannerWorker();
}

PluginScanner* PluginManager::createAudioPluginScanner()
{
    auto* scanner = new PluginScanner (getKnownPlugins());
    return scanner;
}

PluginScanner* PluginManager::getBackgroundAudioPluginScanner()
{
    if (! priv)
        return nullptr;

    if (! priv->scanner)
    {
        priv->scanner.reset (createAudioPluginScanner());
        priv->scanner->addListener (priv.get());
    }

    return priv->scanner.get();
}

bool PluginManager::isScanningAudioPlugins()
{
    return (priv && priv->scanner) ? priv->scanner->isScanning()
                                   : false;
}

AudioPluginInstance* PluginManager::createAudioPlugin (const PluginDescription& desc, String& errorMsg)
{
    return getAudioPluginFormats().createPluginInstance (
                                      desc, priv->sampleRate, priv->blockSize, errorMsg)
        .release();
}

Processor* PluginManager::createGraphNode (const PluginDescription& desc, String& errorMsg)
{
    errorMsg.clear();
    auto& nodes = getNodeFactory();
    if (auto* const plugin = createAudioPlugin (desc, errorMsg))
    {
        plugin->enableAllBuses();
        return NodeFactory::wrap (plugin);
    }

    if (desc.pluginFormatName == "Internal")
    {
        errorMsg.clear();
        if (desc.fileOrIdentifier == "audio.input")
            return new IONode (IONode::audioInputNode);
        else if (desc.fileOrIdentifier == "audio.output")
            return new IONode (IONode::audioOutputNode);
        else if (desc.fileOrIdentifier == "midi.input")
            return new IONode (IONode::midiInputNode);
        else if (desc.fileOrIdentifier == "midi.output")
            return new IONode (IONode::midiOutputNode);
        else
        {
            errorMsg = "Could not create internal node";
            return nullptr;
        }
    }

    if (errorMsg.isNotEmpty() && desc.pluginFormatName != EL_NODE_FORMAT_NAME && desc.pluginFormatName != "LV2")
    {
        return nullptr;
    }

    errorMsg.clear();
    if (desc.pluginFormatName != EL_NODE_FORMAT_NAME && desc.pluginFormatName != "LV2")
    {
        errorMsg = desc.name;
        errorMsg << ": invalid format: " << desc.pluginFormatName;
        return nullptr;
    }

    if (auto* node = nodes.instantiate (desc))
        return node;

    errorMsg = desc.name;
    errorMsg << " not found.";
    return nullptr;
}

AudioPluginFormatManager& PluginManager::getAudioPluginFormats()
{
    return priv->formats;
}

bool PluginManager::isAudioPluginFormatSupported (const String& name) const
{
    auto& fmts = priv->formats;
    for (int i = 0; i < fmts.getNumFormats(); ++i)
        if (fmts.getFormat (i)->getName() == name)
            return true;
    auto& nodes = priv->nodes;
    for (const auto* provider : nodes.providers())
        if (provider->format() == name)
            return true;
    return false;
}

AudioPluginFormat* PluginManager::getAudioPluginFormat (const String& name) const
{
    auto& manager = priv->formats;
    for (int i = 0; i < manager.getNumFormats(); ++i)
    {
        AudioPluginFormat* fmt = manager.getFormat (i);
        if (fmt && fmt->getName() == name)
            return fmt;
    }

    return nullptr;
}

KnownPluginList& PluginManager::getKnownPlugins() { return priv->allPlugins; }
const KnownPluginList& PluginManager::getKnownPlugins() const { return priv->allPlugins; }
const File& PluginManager::getDeadAudioPluginsFile() const { return priv->deadAudioPlugins; }

void PluginManager::saveUserPlugins (ApplicationProperties& settings)
{
    setPropertiesFile (settings.getUserSettings());
    if (auto elm = priv->allPlugins.createXml())
    {
        props->setValue (pluginListKey(), elm.get());
        props->saveIfNeeded();
    }
}

void PluginManager::restoreUserPlugins (ApplicationProperties& settings)
{
    setPropertiesFile (settings.getUserSettings());
    if (props == nullptr)
        return;
    if (auto xml = props->getXmlValue (pluginListKey()))
        restoreUserPlugins (*xml);
    settings.saveIfNeeded();
}

void PluginManager::restoreUserPlugins (const XmlElement& xml)
{
    priv->allPlugins.recreateFromXml (xml);
    scanInternalPlugins();
    priv->updateBlacklistedAudioPlugins();
    if (props == nullptr)
        return;

    if (auto e = priv->allPlugins.createXml())
    {
        props->setValue (pluginListKey(), e.get());
        props->saveIfNeeded();
    }
}

void PluginManager::setPlayConfig (double sampleRate, int blockSize)
{
    priv->sampleRate = sampleRate;
    priv->blockSize = blockSize;
}

void PluginManager::scanAudioPlugins (const StringArray& names)
{
    if (! priv)
        return;

    scanInternalPlugins();

    if (isScanningAudioPlugins())
        return;

    priv->scanAudioPlugins (names);
}

String PluginManager::getCurrentlyScannedPluginName() const
{
    return (priv) ? priv->getScannedPluginName() : String();
}

void PluginManager::scanInternalPlugins()
{
    auto& nodes = priv->nodes;

    auto& known = getKnownPlugins();
    const auto types = known.getTypes();
    for (const auto& t : types)
    {
        if (t.pluginFormatName != EL_NODE_FORMAT_NAME)
            continue;
        known.removeType (t);
        known.removeFromBlacklist (t.fileOrIdentifier);
        known.removeFromBlacklist (t.createIdentifierString());
    }

    OwnedArray<PluginDescription> ds;
    for (const auto& nodeTypeId : nodes.knownIDs())
        nodes.getPluginDescriptions (ds, nodeTypeId);
    for (const auto* const d : ds)
    {
        known.removeType (*d);
        known.removeFromBlacklist (d->fileOrIdentifier);
        known.removeFromBlacklist (d->createIdentifierString());
        known.addType (*d);
    }
}

void PluginManager::getUnverifiedPlugins (const String& formatName, OwnedArray<PluginDescription>& plugins)
{
    priv->getUnverifiedPlugins (formatName, plugins);
    if (plugins.isEmpty())
        priv->searchUnverifiedPlugins (props);
}

void PluginManager::scanFinished()
{
    restoreAudioPlugins (PluginScanner::getWorkerPluginListFile());
    if (auto* scanner = getBackgroundAudioPluginScanner())
        scanner->cancel();
    jassert (! isScanningAudioPlugins());
    sendChangeMessage();
}

void PluginManager::restoreAudioPlugins (const File& file)
{
    if (auto xml = XmlDocument::parse (file))
        restoreUserPlugins (*xml);
}

const File& PluginScanner::getWorkerPluginListFile()
{
    static File _listTempFile;
#if 0
    if (_listTempFile == File())
        _listTempFile = File::createTempFile ("el-pm-worker");
#else
    if (_listTempFile == File())
        _listTempFile = DataPath::applicationDataDir().getChildFile (EL_PLUGIN_SCANNER_SLAVE_LIST_PATH);
#endif
    return _listTempFile;
}

PluginDescription PluginManager::findDescriptionFor (const Node& node) const
{
    PluginDescription desc;

    const String identifierString (node.getProperty (tags::pluginIdentifierString).toString());
    bool wasFound = false;

    if (identifierString.isNotEmpty())
    {
        // fastest, find by identifer string in known plugins
        if (const auto type = getKnownPlugins().getTypeForIdentifierString (
                node.getProperty (tags::pluginIdentifierString).toString()))
        {
            desc = *type;
            wasFound = true;
        }
    }

    if (! wasFound)
    {
        // Manually load and search
        OwnedArray<PluginDescription> types;
        if (auto* format = getAudioPluginFormat (desc.pluginFormatName))
            format->findAllTypesForFile (types, desc.fileOrIdentifier);
        if (! types.isEmpty())
        {
            desc = *types.getFirst();
            wasFound = true;
        }
    }

    if (! wasFound)
    {
        // last resort
        node.getPluginDescription (desc);
    }

    return desc;
}

void PluginManager::saveDefaultNode (const Node& node)
{
    if (! node.isValid())
        return;
    auto desc = findDescriptionFor (node);
    auto file = DataPath::applicationDataDir().getChildFile ("nodes");
    file = file.getChildFile (desc.createIdentifierString());
    file.createDirectory();
    file = file.getChildFile ("default.eln");
    node.writeToFile (file);
}

Node PluginManager::getDefaultNode (const PluginDescription& desc) const
{
    auto file = DataPath::applicationDataDir().getChildFile ("nodes");
    file = file.getChildFile (desc.createIdentifierString());
    file = file.getChildFile ("default.eln");
    if (! file.existsAsFile())
        return Node();
    auto data = Node::parse (file);
    auto node = Node (Node::resetIds (data), false);
    Node::sanitizeProperties (data);
    data.removeProperty (tags::name, nullptr);
    return node;
}

} // namespace element

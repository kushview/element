// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <boost/dll.hpp>

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

#include <errno.h>
extern char* program_invocation_name;

namespace element {
using namespace juce;

namespace detail {
static const char* pluginListKey() { return Settings::pluginListKey; }
/* noop. prevent OS error dialogs from child process */
static void pluginScannerCrashHandler (void*) {}
static File pluginsXmlFile() { return DataPath::applicationDataDir().getChildFile ("plugins.xml"); }

static FileSearchPath readSearchPath (const PropertiesFile& props, const String& f)
{
    const auto key = String (Settings::lastPluginScanPathPrefix) + f;
    return FileSearchPath (props.getValue (key));
}

static File scannerExeFullPath()
{
    auto scannerExe = File::getSpecialLocation (File::currentExecutableFile);

#if JUCE_LINUX
    if (! scannerExe.existsAsFile())
    {
        char* path = (char*) malloc (PATH_MAX);
        if (path != NULL)
        {
            if (readlink ("/proc/self/exe", path, PATH_MAX) > 0)
                scannerExe = File (String (path));
            std::free (path);
        }
    }
#endif

    return scannerExe;
}

static juce::StringArray readDeadMansPedalFile()
{
    const auto file = DataPath::applicationDataDir().getChildFile (EL_DEAD_AUDIO_PLUGINS_FILENAME);
    StringArray lines;
    file.readLines (lines);
    lines.removeEmptyStrings();
    return lines;
}

static void setDeadMansPedalFile (const StringArray& newContents)
{
    auto deadMansPedalFile = DataPath::applicationDataDir().getChildFile (EL_DEAD_AUDIO_PLUGINS_FILENAME);
    if (deadMansPedalFile.getFullPathName().isNotEmpty())
        deadMansPedalFile.replaceWithText (newContents.joinIntoString ("\n"), true, true);
}

static void applyBlacklistingsFromDeadMansPedal (KnownPluginList& list)
{
    // If any plugins have crashed recently when being loaded, move them to the
    // end of the list to give the others a chance to load correctly..
    for (auto& crashedPlugin : readDeadMansPedalFile())
        list.addToBlacklist (crashedPlugin);
}

} // namespace detail

//==============================================================================
class PluginScannerCoordinator : public juce::ChildProcessCoordinator
{
public:
    explicit PluginScannerCoordinator (PluginScanner& o)
        : owner (o)
    {
        launchScanner (0, 0);
    }

    ~PluginScannerCoordinator() {}

    enum class State
    {
        timeout,
        gotResult,
        connectionLost,
    };

    struct Response
    {
        State state;
        std::unique_ptr<XmlElement> xml;
    };

    Response getResponse()
    {
        std::unique_lock<std::mutex> lock { mutex };

        if (! condvar.wait_for (lock, std::chrono::milliseconds { 50 }, [&] { return gotResult || connectionLost; }))
            return { State::timeout, nullptr };

        const auto state = connectionLost ? State::connectionLost : State::gotResult;
        connectionLost = false;
        gotResult = false;

        return { state, std::move (pluginDescription) };
    }

    void handleMessageFromWorker (const MemoryBlock& mb) override
    {
        const std::lock_guard<std::mutex> lock { mutex };
        pluginDescription = juce::parseXML (mb.toString());
        gotResult = true;
        condvar.notify_one();
    }

    void handleConnectionLost() override
    {
        const std::lock_guard<std::mutex> lock { mutex };
        connectionLost = true;
        condvar.notify_one();
    }

private:
    PluginScanner& owner;

    std::mutex mutex;
    std::condition_variable condvar;

    std::unique_ptr<XmlElement> pluginDescription;
    bool connectionLost = false;
    bool gotResult = false;

    bool launchScanner (const int timeout = EL_PLUGIN_SCANNER_DEFAULT_TIMEOUT, const int flags = 0)
    {
        auto scannerExe = owner.scannerExeFile();
        if (! scannerExe.existsAsFile())
        {
            Logger::writeToLog ("Failed to launch plugin scanner.");
            return false;
        }

        Logger::writeToLog (String ("launching plugin scanner: ") + scannerExe.getFullPathName());
        return launchWorkerProcess (scannerExe,
                                    EL_PLUGIN_SCANNER_PROCESS_ID,
                                    timeout,
                                    flags);
    }
};

//==============================================================================
class PluginScannerWorker : public juce::ChildProcessWorker,
                            public juce::AsyncUpdater
{
public:
    PluginScannerWorker()
    {
        SystemStats::setApplicationCrashHandler (detail::pluginScannerCrashHandler);
        auto logfile = DataPath::applicationDataDir().getChildFile ("log/scanner.log");
        logfile.create();
        logger = std::make_unique<juce::FileLogger> (logfile, "Plugin Scanner");
    }

    ~PluginScannerWorker()
    {
    }

    void handleMessageFromCoordinator (const MemoryBlock& mb) override
    {
        if (mb.isEmpty())
            return;

        const std::lock_guard<std::mutex> lock (mutex);

        if (const auto results = doScan (mb); ! results.isEmpty())
        {
            sendResults (results);
        }
        else
        {
            pendingBlocks.emplace (mb);
            triggerAsyncUpdate();
        }
    }

    void handleAsyncUpdate() override
    {
        for (;;)
        {
            const std::lock_guard<std::mutex> lock (mutex);

            if (pendingBlocks.empty())
                return;

            sendResults (doScan (pendingBlocks.front()));
            pendingBlocks.pop();
        }
    }

    OwnedArray<PluginDescription> doScan (const MemoryBlock& block)
    {
        MemoryInputStream stream { block, false };
        const auto formatName = stream.readString();
        const auto identifier = stream.readString();
        return nullptr == plugins->getAudioPluginFormat (formatName)
                   ? scanProvider (formatName, identifier)
                   : scanJuce (formatName, identifier);
    }

    OwnedArray<PluginDescription> scanJuce (const String& formatName, const String& identifier)
    {
        PluginDescription pd;
        pd.pluginFormatName = formatName;
        pd.fileOrIdentifier = identifier;
        pd.uniqueId = pd.deprecatedUid = 0;

        const auto matchingFormat = plugins->getAudioPluginFormat (formatName);

        OwnedArray<PluginDescription> results;

        if (matchingFormat != nullptr
            && (MessageManager::getInstance()->isThisTheMessageThread()
                || matchingFormat->requiresUnblockedMessageThreadDuringCreation (pd)))
        {
            matchingFormat->findAllTypesForFile (results, identifier);
        }

        return results;
    }

    OwnedArray<PluginDescription> scanProvider (const String& format, const String& ID)
    {
        auto& nodes = plugins->getNodeFactory();

        OwnedArray<PluginDescription> results;

        for (auto* p : nodes.providers())
        {
            if (p->format() != format)
                continue;

            if (auto inst = p->create (ID))
            {
                auto d = results.add (new PluginDescription());
                inst->getPluginDescription (*d);
            }

            break;
        }

        return results;
    }

    void sendResults (const OwnedArray<PluginDescription>& results)
    {
        XmlElement xml ("LIST");

        for (const auto& desc : results)
            xml.addChildElement (desc->createXml().release());

        const auto str = xml.toString();
        sendMessageToCoordinator ({ str.toRawUTF8(), str.getNumBytesAsUTF8() });
    }

    void handleConnectionMade() override
    {
        logger->logMessage ("[scanner] connection to coordinator established");
        logger->logMessage ("[scanner] creating global objects");
        settings = std::make_unique<Settings>();
        plugins = std::make_unique<PluginManager>();

        logger->logMessage ("[scanner] setting up formats");
        auto& nf = plugins->getNodeFactory();
        nf.add (new LV2NodeProvider());
        plugins->addDefaultFormats();
        plugins->setPlayConfig (48000.0, 1024);
    }

    void handleConnectionLost() override
    {
        logger->logMessage ("[scanner] connection lost");
        logger.reset();
        settings = nullptr;
        plugins = nullptr;
        JUCEApplication::quit();
    }

private:
    std::unique_ptr<Settings> settings;
    std::unique_ptr<PluginManager> plugins;
    std::mutex mutex;
    std::queue<MemoryBlock> pendingBlocks;
    std::unique_ptr<juce::FileLogger> logger;
};

//==============================================================================
PluginScanner::PluginScanner (PluginManager& manager)
    : _manager (manager),
      list (manager.getKnownPlugins()) {}

PluginScanner::~PluginScanner()
{
    listeners.clear();
    superprocess.reset();
}

void PluginScanner::cancel()
{
    cancelFlag = 1;
}

bool PluginScanner::isScanning() const { return superprocess != nullptr; }

bool PluginScanner::retrieveDescriptions (const String& formatName,
                                          const String& fileOrIdentifier,
                                          OwnedArray<PluginDescription>& result)
{
    if (superprocess == nullptr)
        superprocess = std::make_unique<PluginScannerCoordinator> (*this);

    MemoryBlock block;
    MemoryOutputStream stream { block, true };
    stream.writeString (formatName);
    stream.writeString (fileOrIdentifier);

    if (! superprocess->sendMessageToWorker (block))
        return false;

    using State = PluginScannerCoordinator::State;

    for (;;)
    {
        if (cancelFlag.get() != 0)
            return true;

        const auto response = superprocess->getResponse();

        if (response.state == State::timeout)
            continue;

        if (response.xml != nullptr)
        {
            for (const auto* item : response.xml->getChildIterator())
            {
                auto desc = std::make_unique<PluginDescription>();

                if (desc->loadFromXml (*item))
                    result.add (std::move (desc));
            }
        }

        return (response.state == State::gotResult);
    }
}

File PluginScanner::scannerExeFile() const noexcept
{
    return _scannerExe != File() ? _scannerExe : detail::scannerExeFullPath();
}

void PluginScanner::scanAudioFormat (const String& formatName)
{
    detail::applyBlacklistingsFromDeadMansPedal (list);

    StringArray identifiers;
    std::function<String (const String&)> pluginName = [] (const String& ID) -> juce::String { return ID; };

    if (auto* format = _manager.getAudioPluginFormat (formatName))
    {
        pluginName = [format] (const String& ID) {
            return format->getNameOfPluginFromIdentifier (ID);
        };

        identifiers = format->searchPathsForPlugins (
            detail::readSearchPath (*_manager.props, formatName),
            true,
            false);
    }
    else if (auto* provider = _manager.getProvider (formatName))
    {
        identifiers = provider->findTypes (
            detail::readSearchPath (*_manager.props, formatName),
            true,
            false);
    }

    listeners.call (&Listener::audioPluginScanProgress, 0.0f);

    float step = 1.f;
    for (const auto& ID : identifiers)
    {
        if (cancelFlag.get() != 0)
            return;

        listeners.call (&Listener::audioPluginScanStarted, pluginName (ID));

        if (list.getTypeForFile (ID) || list.getBlacklistedFiles().contains (ID))
            continue;

        OwnedArray<PluginDescription> descriptions;

        auto crashed = detail::readDeadMansPedalFile();
        crashed.removeString (ID);
        crashed.add (ID);
        detail::setDeadMansPedalFile (crashed);

        if (retrieveDescriptions (formatName, ID, descriptions))
        {
            for (auto* desc : descriptions)
                list.addType (*desc);

            // Managed to load without crashing, so remove it from the dead-man's-pedal..
            crashed.removeString (ID);
            detail::setDeadMansPedalFile (crashed);
        }

        if (descriptions.size() == 0 && ! list.getBlacklistedFiles().contains (ID))
            failedIdentifiers.add (ID);

        listeners.call (&Listener::audioPluginScanProgress,
                        step / static_cast<float> (identifiers.size()));
        step += 1.f;
    }
}

void PluginScanner::scanForAudioPlugins (const juce::String& formatName)
{
    const juce::StringArray identifiers { formatName };
    scanForAudioPlugins (identifiers);
}

void PluginScanner::scanForAudioPlugins (const StringArray& formats)
{
    if (! scannerExeFile().existsAsFile())
        return;

    detail::setDeadMansPedalFile ({});
    cancelFlag = 0;

    for (const auto& format : formats)
    {
        scanAudioFormat (format);
        if (cancelFlag.get() != 0)
            break;
    }

    superprocess.reset();
    cancelFlag = 0;

    auto crashed = detail::readDeadMansPedalFile();
    for (const auto& c : failedIdentifiers)
        crashed.add (c);
    crashed.removeDuplicates (false);
    crashed.removeEmptyStrings();
    detail::setDeadMansPedalFile (crashed);
    detail::applyBlacklistingsFromDeadMansPedal (list);
    detail::setDeadMansPedalFile ({});
    failedIdentifiers.clearQuick(); // FIXME: this is a workaround that
        // prevents the UI from showing to
        // many errors about known-crashed
        // plugins
    listeners.call (&Listener::audioPluginScanFinished);
}

//==============================================================================
using UnverifiedPluginMap = HashMap<String, StringArray>;
using UnverifiedPluginPaths = HashMap<String, FileSearchPath>;

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
        if (format == "LV2")
        {
            for (const auto& item : lv2Items)
            {
                if (nullptr != list.getTypeForFile (item.identifier))
                    continue;
                auto desc = plugs.add (new PluginDescription());
                desc->pluginFormatName = "LV2";
                desc->fileOrIdentifier = item.identifier;
                desc->name = item.name;
            }
        }
        else if (plugins.contains (format))
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

    struct Item
    {
        String name;
        String identifier;
    };

    std::vector<Item> lv2Items;

    Atomic<int> cancelFlag;

    void run() override
    {
        cancelFlag.set (0);

        PluginManager pluginManager;
        pluginManager.addDefaultFormats();
        pluginManager.getNodeFactory().add (new LV2NodeProvider());
        auto& manager (pluginManager.getAudioPluginFormats());

        // JUCE Formats.
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

        // Element Node Providers
        lv2Items.clear();
        auto& factory = pluginManager.getNodeFactory();
        for (auto provider : factory.providers())
        {
            if (auto* lv2 = dynamic_cast<LV2NodeProvider*> (provider))
            {
                FileSearchPath fp;
                const auto types = lv2->findTypes (fp, false, false);
                lv2Items.clear();
                lv2Items.reserve ((size_t) types.size());
                for (const auto& uri : types)
                {
                    const auto name = lv2->nameForURI (uri);
                    lv2Items.push_back ({ name, uri });
                }

                lv2Items.shrink_to_fit();
                break;
            }
        }

        cancelFlag.set (0);
    }
};

//==============================================================================
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
        }

        StringArray formatsToScan = names;
        if (formatsToScan.isEmpty())
        {
            for (int i = 0; i < formats.getNumFormats(); ++i)
                if (formats.getFormat (i)->getName() != "Element" && formats.getFormat (i)->canScanForPlugins())
                    formatsToScan.add (formats.getFormat (i)->getName());
            formatsToScan.add ("LV2");
        }

        scanner = std::make_unique<PluginScanner> (owner);
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
    auto* scanner = new PluginScanner (*this);
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
            errorMsg << ": " << desc.fileOrIdentifier;
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
        elm->writeTo (detail::pluginsXmlFile());
}

void PluginManager::restoreUserPlugins (ApplicationProperties& settings)
{
    setPropertiesFile (settings.getUserSettings());
    if (props == nullptr)
        return;

    // transfer old plugins to new.
    if (auto xml = props->getXmlValue (detail::pluginListKey()))
    {
        xml->writeTo (detail::pluginsXmlFile());
        props->removeValue (detail::pluginListKey());
    }

    if (auto xml = XmlDocument::parse (detail::pluginsXmlFile()))
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
    {
        nodes.getPluginDescriptions (ds, nodeTypeId);
    }
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
    // this file is deprecated and should just be removed.
    if (PluginScanner::getWorkerPluginListFile().existsAsFile())
        PluginScanner::getWorkerPluginListFile().deleteFile();
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
        desc.pluginFormatName = node.getProperty (tags::format).toString();
        desc.fileOrIdentifier = node.getProperty (tags::identifier).toString();

        DBG ("[element] manual load: " << desc.pluginFormatName << " : " << desc.fileOrIdentifier);

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

NodeProvider* PluginManager::getProvider (const String& format) noexcept
{
    for (auto provider : getNodeFactory().providers())
        if (provider->format() == format)
            return provider;
    return nullptr;
}

} // namespace element

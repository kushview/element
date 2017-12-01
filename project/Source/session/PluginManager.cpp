/*
    PluginManager.cpp - This file is part of Element
    Copyright (C) 2014-2018  Kushview, LLC.  All rights reserved.
*/

#include "session/PluginManager.h"
#include "DataPath.h"
#include "Settings.h"

#define EL_DEAD_AUDIO_PLUGINS_FILENAME          "DeadAudioPlugins.txt"

#define EL_PLUGIN_SCANNER_WAITING_STATE         "waiting"
#define EL_PLUGIN_SCANNER_READY_STATE           "ready"

#define EL_PLUGIN_SCANNER_READY_ID              "ready"
#define EL_PLUGIN_SCANNER_START_ID              "start"
#define EL_PLUGIN_SCANNER_FINISHED_ID           "finished"

#define EL_PLUGIN_SCANNER_DEFAULT_TIMEOUT       20000  // 20 Seconds

namespace Element {

class PluginScannerMaster : public ChildProcessMaster,
                            public AsyncUpdater
{
public:
    explicit PluginScannerMaster (PluginScanner& o) : owner(o) { }
    ~PluginScannerMaster() { }
    
    bool startScanning (const StringArray& names = StringArray())
    {
        if (isRunning())
            return true;
        
        {
            ScopedLock sl (lock);
            slaveState  = String();
            running     = false;
            formatNames = names;
        }
        
        const bool res = launchScanner();
        
        {
            ScopedLock sl (lock);
            running = res;
        }
        
        return res;
    }
    
    void handleMessageFromSlave (const MemoryBlock& mb) override
    {
        const auto data (mb.toString());
        const auto type (data.upToFirstOccurrenceOf (":", false, false));
        const auto message (data.fromFirstOccurrenceOf (":", false, false));
        
        if (type == "state")
        {
            ScopedLock sl (lock);
            const String lastState = slaveState;
            slaveState = message;
            if (lastState != slaveState) {
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
            float newProgress = (float) var(message);
            owner.listeners.call (&PluginScanner::Listener::audioPluginScanProgress, newProgress);
            ScopedLock sl (lock);
            progress = newProgress;
        }
    }
    
    void handleConnectionLost() override
    {
        // this probably will happen when a plugin crashes.
        {
            ScopedLock sl (lock);
            running = false;
        }
        
        triggerAsyncUpdate();
    }
    
    void handleAsyncUpdate() override
    {
        if (slaveState == "ready" && isRunning())
        {
            String msg = "scan:"; msg << formatNames.joinIntoString(",");
            MemoryBlock mb (msg.toRawUTF8(), msg.length());
            sendMessageToSlave (mb);
        }
        else if (slaveState == "scanning")
        {
            if (! isRunning())
            {
                DBG("[EL] a plugin crashed or timed out during scan");
                const bool res = launchScanner();
                ScopedLock sl (lock);
                running = res;
            }
            else
            {
                DBG("[EL] scanning... and running....");
            }
        }
        else if (slaveState == "finished")
        {
            DBG("[EL] slave finished scanning");
            owner.listeners.call (&PluginScanner::Listener::audioPluginScanFinished);
        }
        else
        {
            DBG("[EL] invalid slave state: " << slaveState);
        }
    }
    
    const String getSlaveState() const
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
    
private:
    PluginScanner& owner;

    CriticalSection lock;
    bool running    = false;
    float progress  = 0.f;
    String slaveState;
    StringArray formatNames;
    StringArray faileFiles;
    
    String pluginBeingScanned;
    
    void resetScannerVariables()
    {
        ScopedLock sl (lock);
        pluginBeingScanned = String();
        progress = -1.f;
    }
    
    bool launchScanner (const int timeout = EL_PLUGIN_SCANNER_DEFAULT_TIMEOUT, const int flags = 0)
    {
        return launchSlaveProcess (File::getSpecialLocation (File::currentExecutableFile),
                                   EL_PLUGIN_SCANNER_PROCESS_ID, timeout, flags);
    }
};

class PluginScannerSlave : public ChildProcessSlave
{
public:
    PluginScannerSlave() { }
    ~PluginScannerSlave() { }
    
    void handleMessageFromMaster (const MemoryBlock& mb) override
    {
        const auto data (mb.toString());
        const auto type (data.upToFirstOccurrenceOf (":", false, false));
        const auto message (data.fromFirstOccurrenceOf (":", false, false));
        
        if (type == "scan")
        {
            sendState ("scanning");
            
            const auto formats (StringArray::fromTokens (message.trim(), ",", "'"));

            for (const auto& format : formats) {
                scanFor (format);
            }
            
            sendState ("finished");
        }
    }
    
    void handleConnectionMade() override
    {
        settings    = new Settings();
        plugins     = new PluginManager();
        plugins->addDefaultFormats();
        plugins->restoreUserPlugins (*settings);
        sendState (EL_PLUGIN_SCANNER_READY_ID);
    }
    
    void handleConnectionLost() override
    {
        settings    = nullptr;
        plugins     = nullptr;
        scanner     = nullptr;
        exit(0);
    }

private:
    ScopedPointer<Settings> settings;
    ScopedPointer<PluginManager> plugins;
    ScopedPointer<PluginDirectoryScanner> scanner;
    String fileOrIdentifier;
    
    bool sendState (const String& state)
    {
        return sendString ("state", state);
    }
    
    bool sendString (const String& type, const String& message)
    {
        String data = type; data << ":" << message.trim();
        MemoryBlock mb (data.toRawUTF8(), data.length());
        return sendMessageToMaster (mb);
    }
    
    bool doNextScan()
    {
        sendString ("name", scanner->getNextPluginFileThatWillBeScanned());
        if (scanner->scanNextFile (true, fileOrIdentifier))
        {
            sendString ("progress", String (scanner->getProgress()));
            plugins->saveUserPlugins (*settings);
            settings->saveIfNeeded();
            return true;
        }
        
        return false;
    }
    
    void scanFor (const String& formatName)
    {
        if (plugins == nullptr || settings == nullptr)
            return;
        if (auto* format = plugins->getAudioPluginFormat (formatName))
            scanFor (*format);
    }
    
    void scanFor (AudioPluginFormat& format)
    {
        if (plugins == nullptr || settings == nullptr)
            return;
        
        const auto key = String(settings->lastPluginScanPathPrefix) + format.getName();
        FileSearchPath path (settings->getUserSettings()->getValue (key));
        scanner = new PluginDirectoryScanner (plugins->availablePlugins(), format,
                                              path, true, plugins->getDeadAudioPluginsFile(),
                                              false);
        
        while (doNextScan()) { }
    }
};

// MARK: Plugin Scanner

PluginScanner::PluginScanner() { }
PluginScanner::~PluginScanner()
{
    listeners.clear();
    master = nullptr;
}

void PluginScanner::cancel() { master = nullptr; }

bool PluginScanner::isScanning() const { return master && master->isRunning(); }

void PluginScanner::scanForAudioPlugins (const juce::String &formatName)
{
    scanForAudioPlugins (StringArray ({ formatName }));
}

void PluginScanner::scanForAudioPlugins (const StringArray& formats)
{
    if (master)
    {
        master = nullptr;
    }
    
    master = new PluginScannerMaster (*this);
    master->startScanning (formats);
}

void PluginScanner::timerCallback()
{
}
    
// MARK: Plugin Manager
    
class PluginManager::Private : public PluginScanner::Listener
{
public:
    Private (PluginManager& o) : owner (o)
    {
        deadAudioPlugins = DataPath::applicationDataDir().getChildFile (EL_DEAD_AUDIO_PLUGINS_FILENAME);
    }

    ~Private() {  }

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
    
private:
    friend class PluginManager;
    PluginManager& owner;
    AudioPluginFormatManager formats;
    KnownPluginList allPlugins;
    File deadAudioPlugins;
    
   #if ELEMENT_LV2_PLUGIN_HOST
    OptionalPtr<LV2World> lv2;
    OptionalPtr<SymbolMap> symbols;
   #endif

    double sampleRate   = 44100.0;
    int    blockSize    = 512;
    
    ScopedPointer<PluginScanner> scanner;
    
    void scanAudioPlugins (const StringArray& names)
    {
        if (scanner)
        {
            scanner->cancel();
            scanner = nullptr;
        }
    
        StringArray formatsToScan = names;
        if (formatsToScan.isEmpty())
            for (int i = 0; i < formats.getNumFormats(); ++i)
                if (formats.getFormat(i)->getName() != "Element" && formats.getFormat(i)->canScanForPlugins())
                    formatsToScan.add (formats.getFormat(i)->getName());
   
        scanner = new PluginScanner();
        scanner->addListener (this);
        scanner->scanForAudioPlugins (formatsToScan);
    }
    
    void audioPluginScanFinished() override
    {
        owner.scanFinished();
    }
    
    void audioPluginScanStarted (const String& plugin) override
    {
        DBG("[EL] plugin scanned in background: " << plugin);
    }
};

PluginManager::PluginManager()
{
    priv = new Private (*this);
   #if ELEMENT_LV2_PLUGIN_HOST
    priv->symbols.setOwned (new SymbolMap ());
    priv->lv2.setOwned (new LV2World());
   #endif
}

PluginManager::~PluginManager()
{
    priv = nullptr;
}

void PluginManager::addDefaultFormats()
{
    formats().addDefaultFormats();
   #if ELEMENT_LV2_PLUGIN_HOST
    addFormat (new LV2PluginFormat (*priv->lv2));
   #endif
}

void PluginManager::addFormat (AudioPluginFormat* fmt)
{
    getAudioPluginFormats().addFormat (fmt);
}

ChildProcessSlave* PluginManager::createAudioPluginScannerSlave()
{
    return new PluginScannerSlave();
}

PluginScanner* PluginManager::createAudioPluginScanner()
{
    auto* scanner = new PluginScanner();
    return scanner;
}

PluginScanner* PluginManager::getBackgroundAudioPluginScanner()
{
    return (priv != nullptr) ? priv->scanner.get() : nullptr;
}

bool PluginManager::isScanningAudioPlugins()
{
    return (priv && priv->scanner) ? priv->scanner->isScanning()
                                   : false;
}

AudioPluginInstance* PluginManager::createAudioPlugin (const PluginDescription& desc, String& errorMsg)
{
    return formats().createPluginInstance (desc, priv->sampleRate, priv->blockSize, errorMsg);
}

Processor* PluginManager::createPlugin (const PluginDescription &desc, String &errorMsg)
{
    if (AudioPluginInstance* instance = createAudioPlugin (desc, errorMsg))
        return dynamic_cast<Processor*> (instance);
    return nullptr;
}

AudioPluginFormat* PluginManager::format (const String& name)
{
    for (int i = 0; i < formats().getNumFormats(); ++i)
    {
        AudioPluginFormat* fmt = formats().getFormat (i);
        if (fmt && fmt->getName() == name)
            return fmt;
    }

    return nullptr;
}

AudioPluginFormatManager& PluginManager::formats()
{
    return priv->formats;
}

KnownPluginList& PluginManager::availablePlugins() { return priv->allPlugins; }
const File& PluginManager::getDeadAudioPluginsFile() const { return priv->deadAudioPlugins; }

static const char* pluginListKey()  { return Settings::pluginListKey; }


void PluginManager::saveUserPlugins (ApplicationProperties& settings)
{
    ScopedXml elm (priv->allPlugins.createXml());
	settings.getUserSettings()->setValue (pluginListKey(), elm.get());
    settings.saveIfNeeded();
}

void PluginManager::restoreUserPlugins (ApplicationProperties& settings)
{
	if (ScopedXml xml = settings.getUserSettings()->getXmlValue (pluginListKey()))
    {
		restoreUserPlugins (*xml);
        if (priv->updateBlacklistedAudioPlugins())
            saveUserPlugins (settings);
    }
    
    settings.saveIfNeeded();
}

void PluginManager::restoreUserPlugins (const XmlElement& xml)
{
	priv->allPlugins.recreateFromXml (xml);
	scanInternalPlugins();
}

void PluginManager::setPlayConfig (double sampleRate, int blockSize)
{
    priv->sampleRate = sampleRate;
    priv->blockSize  = blockSize;
}

void PluginManager::scanAudioPlugins (const StringArray& names)
{
    if (! priv)
        return;

    if (isScanningAudioPlugins())
        return;
    
    priv->scanAudioPlugins (names);
}

void PluginManager::scanInternalPlugins()
{
    for (int i = 0; i < formats().getNumFormats(); ++i)
    {
        auto* format = formats().getFormat (i);
        
        if (format->getName() != "Element")
            continue;
        
        for (int j = priv->allPlugins.getNumTypes(); --j >= 0;)
            if (priv->allPlugins.getType(j)->pluginFormatName == "Element")
                priv->allPlugins.removeType (j);
        
        PluginDirectoryScanner scanner (availablePlugins(), *format,
                                        format->getDefaultLocationsToSearch(),
                                        true, priv->deadAudioPlugins, false);
        
        String name;
        while (scanner.scanNextFile (true, name)) {}
        
        break;
    }
}
void PluginManager::getUnverifiedPlugins (const String& formatName, OwnedArray<PluginDescription>& plugins)
{
    if (auto* format = getAudioPluginFormat (formatName))
    {
        auto& list (availablePlugins());
        
        FileSearchPath path;
        if (props)
        {
            const auto key = String(Settings::lastPluginScanPathPrefix) + format->getName();
            path = FileSearchPath (props->getValue (key));
        }
        
        path.addPath (format->getDefaultLocationsToSearch());
        
        const auto files = format->searchPathsForPlugins (path, true);
        for (const auto& file: files)
        {
            if (nullptr != list.getTypeForFile (file))
                continue;
            
            auto* desc = plugins.add (new PluginDescription());
            desc->pluginFormatName = formatName;
            desc->fileOrIdentifier = file;
        }
    }
}

void PluginManager::scanFinished()
{
    if (priv->scanner)
        priv->scanner = nullptr;

    jassert(! isScanningAudioPlugins());

    if (props && props->reload())
        if (ScopedXml xml = props->getXmlValue (pluginListKey()))
            restoreUserPlugins (*xml);

    sendChangeMessage();
}

}

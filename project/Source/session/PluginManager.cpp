/*
    PluginManager.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#include "session/PluginManager.h"
#include "DataPath.h"
#include "Settings.h"

#define EL_DEAD_AUDIO_PLUGINS_FILENAME  "DeadAudioPlugins.txt"
#define EL_PLUGIN_SCANNER_START_ID      "start"
#define EL_PLUGIN_SCANNER_FINISHED_ID   "finished"

namespace Element {

class PluginScannerMaster : public ChildProcessMaster,
                            public AsyncUpdater
{
public:
    explicit PluginScannerMaster (PluginManager& o) : owner(o) { }
    ~PluginScannerMaster() { }
    
    bool startScanning (const StringArray& names = StringArray())
    {
        if (running)
            return true;
        
        slaveState = String();
        running = launchScanner();
        return running;
    }
    
    void handleMessageFromSlave (const MemoryBlock& mb) override
    {
        const String message (mb.toString());
        const String lastState = slaveState;
        if (message == EL_PLUGIN_SCANNER_FINISHED_ID)
        {
            slaveState = "complete";
        }
        else
        {
            slaveState = "scanning";
        }
        
        DBG("scanner message: " << message);
        if (lastState != slaveState)
            triggerAsyncUpdate();
    }
    
    void handleConnectionLost() override
    {
        // this probably will happen when a plugin crashes.
        triggerAsyncUpdate();
    }
    
    void handleAsyncUpdate() override
    {
        if (slaveState == "scanning" && running)
        {
            DBG("a plugin crashed or something, still scanning......");
            running = launchScanner();
        }
        else if (slaveState == "complete")
        {
            DBG("Slave finished scanning");
            running = false;
            owner.scanFinished(); // will delete this object
        }
        else
        {
            running = false;
            DBG("slave quit at state: " << slaveState);
            owner.scanFinished(); // will delete this object
        }
    }
    
    bool isScanning() const { return running; }
    
private:
    PluginManager& owner;
    bool running    = false;
    float progress  = 0.f;
    String slaveState;
    
    bool launchScanner (const int timeout = 2000, const int flags = 0)
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
    
    void handleMessageFromMaster (const MemoryBlock&) override { }
    
    void handleConnectionMade() override
    {
        settings    = new Settings();
        plugins     = new PluginManager();
        plugins->addDefaultFormats();
        plugins->restoreUserPlugins (*settings);
        
        sendString (EL_PLUGIN_SCANNER_START_ID);
        Thread::sleep (1000);
        
        auto& formats (plugins->getAudioPluginFormats());
        for (int i = 0; i < formats.getNumFormats(); ++i)
        {
            auto* format = formats.getFormat (i);
            if (! format->canScanForPlugins())
                continue;
            
            const auto key = String(settings->lastPluginScanPathPrefix) + format->getName();
            FileSearchPath path (settings->getUserSettings()->getValue (key));
            
            PluginDirectoryScanner scanner (plugins->availablePlugins(), *format,
                                            path, true, plugins->getDeadAudioPluginsFile(),
                                            false);
            String name;
            
            DBG("[EL] Scanning for " << format->getName() << " plugins...");
            while (scanner.scanNextFile (true, name))
            {
                
            }
        }
        
        plugins->saveUserPlugins (*settings);
        settings->saveIfNeeded();
        sendString (EL_PLUGIN_SCANNER_FINISHED_ID);
    }
    
    void handleConnectionLost() override
    {
        settings = nullptr;
        plugins = nullptr;
        exit(1);
    }
    
    void sendString (const String& state)
    {
        MemoryBlock mb (state.toRawUTF8(), state.length());
        sendMessageToMaster (mb);
    }
    
private:
    ScopedPointer<Settings> settings;
    ScopedPointer<PluginManager> plugins;
};

class PluginManager::Private
{
public:
    Private()
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
    
    KnownPluginList allPlugins;
    File deadAudioPlugins;
    AudioPluginFormatManager formats;

   #if ELEMENT_LV2_PLUGIN_HOST
    OptionalPtr<LV2World> lv2;
    OptionalPtr<SymbolMap> symbols;
   #endif

    double sampleRate   = 44100.0;
    int    blockSize    = 512;
    
    ScopedPointer<PluginScannerMaster> master;
};

PluginManager::PluginManager()
{
    priv = new Private();
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

bool PluginManager::isScanningAudioPlugins() { return (priv && priv->master) ? priv->master->isScanning() : false; }
    
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

static const char* pluginListKey()
{
   #if JUCE_MAC
	return Settings::pluginListKey;
   #elif JUCE_64BIT
	return Settings::pluginListKey64;
   #else
	return Settings::pluginListKey;
   #endif
}

void PluginManager::saveUserPlugins (ApplicationProperties& settings)
{
    ScopedXml elm (priv->allPlugins.createXml());
	settings.getUserSettings()->setValue (pluginListKey(), elm.get());
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
    if (! priv) return;
    if (isScanningAudioPlugins())
        return;
    if (! priv->master)
        priv->master = new PluginScannerMaster (*this);
    priv->master->startScanning (names);
}
    
void PluginManager::scanInternalPlugins()
{
    for (int i = 0; i < formats().getNumFormats(); ++i)
    {
        auto* format = formats().getFormat (i);
        
        if (format->getName() != "Element")
            continue;
        
        PluginDirectoryScanner scanner (availablePlugins(), *format,
                                        format->getDefaultLocationsToSearch(),
                                        true, priv->deadAudioPlugins, false);
        
        String name;
        while (scanner.scanNextFile (true, name)) {}
        
        break;
    }
}

void PluginManager::scanFinished()
{
    if (priv->master)
        priv->master = nullptr;
    
    jassert(! isScanningAudioPlugins());
    
    if (props && props->reload())
        if (ScopedXml xml = props->getXmlValue (pluginListKey()))
            restoreUserPlugins (*xml);

    sendChangeMessage();
}

}

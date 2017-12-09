/*
    PluginManager.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
 */

#pragma once

#include "ElementApp.h"

#define EL_PLUGIN_SCANNER_PROCESS_ID    "pspelbg"

namespace Element {

class PluginScannerMaster;
class PluginScanner;

class PluginManager : public ChangeBroadcaster
{
public:
    PluginManager();
    ~PluginManager();

    void addDefaultFormats();
    void addFormat (AudioPluginFormat*);

    const File& getDeadAudioPluginsFile() const;
    KnownPluginList& availablePlugins();

    AudioPluginFormatManager& getAudioPluginFormats() { return formats(); }
    AudioPluginFormatManager& formats();
    
    AudioPluginFormat* getAudioPluginFormat (const String& formatName) { return format (formatName); }
    AudioPluginFormat* format (const String& formatName);
    
    template<class FormatType>
    inline FormatType* format()
    {
        auto& f (getAudioPluginFormats());
        for (int i = 0; i < f.getNumFormats(); ++i)
            if (FormatType* fmt = dynamic_cast<FormatType*> (f.getFormat (i)))
                return fmt;
        return nullptr;
    }
    
    /** creates a child process slave used in start up */
    ChildProcessSlave* createAudioPluginScannerSlave();
    
    /** creates a new plugin scanner for use by a third party, e.g. plugin manager UI */
    PluginScanner* createAudioPluginScanner();
    
    /** gets the internal plugins scanner used for background scanning */
    PluginScanner* getBackgroundAudioPluginScanner();
    
    /** Scans for all audio plugin types using a child process */
    void scanAudioPlugins (const StringArray& formats = StringArray());
    
    /** true if a scan is in progress using the child process */
    bool isScanningAudioPlugins();
    
	/** returns the name of the currently scanned plugin. This value
	    is not suitable for use in loading plugins */
	String getCurrentlyScannedPluginName() const;
    /** Looks for new or updated internal/element plugins */
    void scanInternalPlugins();
    
    /** Save the known plugins to user settings */
    void saveUserPlugins (ApplicationProperties&);
    
    /** Restore user plugins. Will also scan internal plugins so they don't get removed
        by accident */
    void restoreUserPlugins (ApplicationProperties&);

    /** Restore user plugins. Will also scan internal plugins so they don't get removed
        by accident */
    void restoreUserPlugins (const XmlElement& xml);

    AudioPluginInstance* createAudioPlugin (const PluginDescription& desc, String& errorMsg);
    Processor *createPlugin (const PluginDescription& desc, String& errorMsg);

    void setPlayConfig (double sampleRate, int blockSize);

    /** Give a properties file to be used when settings aren;t available. FIXME */
    void setPropertiesFile (PropertiesFile* pf) {
        props = pf;
    }
    
    /** This will get a possible list of plugins. Trying to load this might fail */
    void getUnverifiedPlugins (const String& formatName, OwnedArray<PluginDescription>& plugins);
    
    /** Restore User Plugins From a file */
    void restoreAudioPlugins (const File&);

private:
    PropertiesFile* props = nullptr;
    class Private;
    ScopedPointer<Private> priv;
    
    friend class PluginScannerMaster;
    void scanFinished();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginManager);
};

class PluginScanner : private Timer
{
public:
    PluginScanner (KnownPluginList&);
    ~PluginScanner();
    
    class Listener
    {
    public:
        Listener () { }
        virtual ~Listener() { }
        
        virtual void audioPluginScanFinished() { }
        virtual void audioPluginScanProgress (const float progress) { ignoreUnused (progress); }
        virtual void audioPluginScanStarted (const String& name) { }
    };
    
    static const File& getSlavePluginListFile();
    
    /** scan for plugins of type */
    void scanForAudioPlugins (const String& formatName);
    
    /** Scan for plugins of multiple types */
    void scanForAudioPlugins (const StringArray& formats);
    
    /** Cancels the current scan operation */
    void cancel();

    /** is scanning */
    bool isScanning() const;
    
    /** Add a listener */
    void addListener (Listener* listener)       { listeners.add (listener); }

    /** Remove a listener */
    void removeListener (Listener* listener)    { listeners.remove (listener); }
    
    /** Returns a list of plugins that failed to load */
    const StringArray& getFailedFiles() const { return failedIdentifiers; }

private:
    friend class PluginScannerMaster;
    friend class Timer;
    ScopedPointer<PluginScannerMaster> master;
    ListenerList<Listener> listeners;
    StringArray failedIdentifiers;
    KnownPluginList& list;
    void timerCallback() override;
};
    
}

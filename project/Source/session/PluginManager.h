/*
    PluginManager.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
 */

#pragma once

#include "ElementApp.h"

#define EL_PLUGIN_SCANNER_PROCESS_ID    "pspelbg"

namespace Element {

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
    
    /** Scans for all audio plugin types using a child process */
    void scanAudioPlugins (const StringArray& formats = StringArray());
    
    /** true if a scan is in progress using the child process */
    bool isScanningAudioPlugins();
    
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
    
private:
    PropertiesFile* props = nullptr;
    class Private;
    ScopedPointer<Private> priv;
    
    friend class PluginScannerMaster;
    void scanFinished();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginManager);
};

}

/*
    PluginManager.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
 */

#pragma once

#include "ElementApp.h"

namespace Element {

class PluginManager
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
        for (int i = 0; i < formats().getNumFormats(); ++i)
            if (FormatType* fmt = dynamic_cast<FormatType*> (formats().getFormat (i)))
                return fmt;
        return nullptr;
    }

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

private:
    class Private;
    ScopedPointer<Private> priv;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginManager);
};

}

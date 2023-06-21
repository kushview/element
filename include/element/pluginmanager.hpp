// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/audio_processors.hpp>

#define EL_PLUGIN_SCANNER_PROCESS_ID "pspelbg"

namespace element {

class ChildProcessSlave;
class NodeObject;
class Node;
class NodeFactory;
class PluginScannerMaster;
class PluginScanner;

// FIXME:
using namespace juce;

class PluginManager : public ChangeBroadcaster {
public:
    PluginManager();
    ~PluginManager();

    /** Add default plugin formats */
    void addDefaultFormats();

    /** Add a plugin format */
    void addFormat (AudioPluginFormat*);

    /** Get the dead mans pedal file */
    const File& getDeadAudioPluginsFile() const;

    /** Access to the main known plugins list */
    KnownPluginList& getKnownPlugins();
    const KnownPluginList& getKnownPlugins() const;

    /** Scan/Add a description to the known plugins */
    void addToKnownPlugins (const PluginDescription& desc);

    /** Returns the audio plugin format manager */
    AudioPluginFormatManager& getAudioPluginFormats();

    /** Returns true if an audio plugin format is supported */
    bool isAudioPluginFormatSupported (const String&) const;

    /** Returns an audio plugin format by name */
    AudioPluginFormat* getAudioPluginFormat (const String& formatName) const;

    /** Returns an audio plugin format by type */
    template <class FormatType>
    inline FormatType* format()
    {
        auto& f (getAudioPluginFormats());
        for (int i = 0; i < f.getNumFormats(); ++i)
            if (FormatType* fmt = dynamic_cast<FormatType*> (f.getFormat (i)))
                return fmt;
        return nullptr;
    }

    /** Returns the node factory. */
    NodeFactory& getNodeFactory();

    /** creates a child process slave used in start up */
    element::ChildProcessSlave* createAudioPluginScannerSlave();

    /** creates a new plugin scanner for use by a third party, e.g. plugin manager UI */
    PluginScanner* createAudioPluginScanner();

    /** gets the internal plugins scanner used for background scanning */
    PluginScanner* getBackgroundAudioPluginScanner();

    /** Scans for all audio plugin types using a child process */
    void scanAudioPlugins (const StringArray& formats = StringArray());

    /** Returns true if a scan is in progress using the child process */
    bool isScanningAudioPlugins();

    /** Returns the name of the currently scanned plugin. This value
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
    NodeObject* createGraphNode (const PluginDescription& desc, String& errorMsg);

    /** Set the play config used when instantiating plugins */
    void setPlayConfig (double sampleRate, int blockSize);

    /** Give a properties file to be used when settings aren't available. FIXME */
    void setPropertiesFile (PropertiesFile* pf) { props = pf; }

    /** Search for unverified plugins in background thread */
    void searchUnverifiedPlugins();

    /** This will get a possible list of plugins. Trying to load this might fail */
    void getUnverifiedPlugins (const String& formatName, OwnedArray<PluginDescription>& plugins);

    /** Restore User Plugins From a file */
    void restoreAudioPlugins (const File&);

    /** Get a PluginDescription for a Node */
    PluginDescription findDescriptionFor (const Node&) const;

    /** Saves the default node state */
    void saveDefaultNode (const Node& node);

    /** Returns the last saved default node state */
    Node getDefaultNode (const PluginDescription& desc) const;

private:
    PropertiesFile* props = nullptr;
    class Private;
    std::unique_ptr<Private> priv;

    friend class PluginScannerMaster;
    void scanFinished();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginManager);
};

class PluginScanner : private Timer {
public:
    PluginScanner (KnownPluginList&);
    ~PluginScanner();

    class Listener {
    public:
        Listener() {}
        virtual ~Listener() {}

        virtual void audioPluginScanFinished() {}
        virtual void audioPluginScanProgress (const float progress) { ignoreUnused (progress); }
        virtual void audioPluginScanStarted (const String& name) {}
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
    void addListener (Listener* listener) { listeners.add (listener); }

    /** Remove a listener */
    void removeListener (Listener* listener) { listeners.remove (listener); }

    /** Returns a list of plugins that failed to load */
    const StringArray& getFailedFiles() const { return failedIdentifiers; }

private:
    friend class PluginScannerMaster;
    friend class Timer;
    std::unique_ptr<PluginScannerMaster> master;
    ListenerList<Listener> listeners;
    StringArray failedIdentifiers;
    KnownPluginList& list;
    void timerCallback() override;
};

} // namespace element

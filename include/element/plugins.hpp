// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/audio_processors.hpp>

#define EL_PLUGIN_SCANNER_PROCESS_ID "pspelbg"

namespace element {

class ChildProcessSlave;
class Processor;
class Node;
class NodeFactory;
class NodeProvider;
class PluginScannerCoordinator;
class PluginScanner;

class PluginManager : public juce::ChangeBroadcaster {
public:
    PluginManager();
    ~PluginManager();

    /** Add default plugin formats */
    void addDefaultFormats();

    /** Add a plugin format */
    void addFormat (juce::AudioPluginFormat*);

    /** Get the dead mans pedal file */
    const juce::File& getDeadAudioPluginsFile() const;

    /** Access to the main known plugins list */
    juce::KnownPluginList& getKnownPlugins();
    const juce::KnownPluginList& getKnownPlugins() const;

    /** Scan/Add a description to the known plugins */
    void addToKnownPlugins (const juce::PluginDescription& desc);

    /** Returns the audio plugin format manager */
    juce::AudioPluginFormatManager& getAudioPluginFormats();

    /** Returns true if an audio plugin format is supported */
    bool isAudioPluginFormatSupported (const juce::String&) const;

    /** Returns an audio plugin format by name */
    juce::AudioPluginFormat* getAudioPluginFormat (const juce::String& formatName) const;

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

    /** Returns the default search path for a given format. */
    juce::FileSearchPath defaultSearchPath (juce::StringRef format) const noexcept;

    /** creates a child process slave used in start up */
    juce::ChildProcessWorker* createAudioPluginScannerWorker();

    /** creates a new plugin scanner for use by a third party, e.g. plugin manager UI */
    PluginScanner* createAudioPluginScanner();

    /** gets the internal plugins scanner used for background scanning */
    PluginScanner* getBackgroundAudioPluginScanner();

    /** Scans for all audio plugin types using a child process */
    void scanAudioPlugins (const juce::StringArray& formats = juce::StringArray());

    /** Returns true if a scan is in progress using the child process */
    bool isScanningAudioPlugins();

    /** Returns the name of the currently scanned plugin. This value
	    is not suitable for use in loading plugins */
    juce::String getCurrentlyScannedPluginName() const;

    /** Looks for new or updated internal/element plugins */
    void scanInternalPlugins();

    /** Save the known plugins to user settings */
    void saveUserPlugins (juce::ApplicationProperties&);

    /** Restore user plugins. Will also scan internal plugins so they don't get removed
        by accident */
    void restoreUserPlugins (juce::ApplicationProperties&);

    /** Restore user plugins. Will also scan internal plugins so they don't get removed
        by accident */
    void restoreUserPlugins (const juce::XmlElement& xml);

    juce::AudioPluginInstance* createAudioPlugin (const juce::PluginDescription& desc, juce::String& errorMsg);
    Processor* createGraphNode (const juce::PluginDescription& desc, juce::String& errorMsg);

    /** Set the play config used when instantiating plugins */
    void setPlayConfig (double sampleRate, int blockSize);

    /** Give a properties file to be used when settings aren't available. FIXME */
    void setPropertiesFile (juce::PropertiesFile* pf) { props = pf; }

    /** Search for unverified plugins in background thread */
    void searchUnverifiedPlugins();

    /** This will get a possible list of plugins. Trying to load this might fail */
    void getUnverifiedPlugins (const juce::String& formatName, juce::OwnedArray<juce::PluginDescription>& plugins);

    /** Restore User Plugins From a file */
    void restoreAudioPlugins (const juce::File&);

    /** Get a juce:: PluginDescription for a Node */
    juce::PluginDescription findDescriptionFor (const Node&) const;

    /** Saves the default node state */
    void saveDefaultNode (const Node& node);

    /** Returns the last saved default node state */
    Node getDefaultNode (const juce::PluginDescription& desc) const;

    /** Get the first node factory by format. e.g. "LV2" */
    NodeProvider* getProvider (const juce::String& format) noexcept;

private:
    friend class PluginScanner;
    juce::PropertiesFile* props = nullptr;
    class Private;
    std::unique_ptr<Private> priv;

    friend class PluginScannerCoordinator;
    void scanFinished();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginManager)
};

class PluginScanner final {
public:
    PluginScanner (PluginManager& manager);
    ~PluginScanner();

    class Listener {
    public:
        Listener() {}
        virtual ~Listener() {}

        virtual void audioPluginScanFinished() {}
        virtual void audioPluginScanProgress (const float progress) { juce::ignoreUnused (progress); }
        virtual void audioPluginScanStarted (const juce::String& name) {}
    };

    static const juce::File& getWorkerPluginListFile();

    /** scan for plugins of type */
    void scanForAudioPlugins (const juce::String& formatName);

    /** Scan for plugins of multiple types */
    void scanForAudioPlugins (const juce::StringArray& formats);

    /** Cancels the current scan operation if possible. */
    void cancel();

    /** is scanning */
    bool isScanning() const;

    /** Add a listener */
    void addListener (Listener* listener) { listeners.add (listener); }

    /** Remove a listener */
    void removeListener (Listener* listener) { listeners.remove (listener); }

    /** Returns a list of plugins that failed to load */
    const juce::StringArray& getFailedFiles() const { return failedIdentifiers; }

    /** Returns the scanner exe to use for out-of-process scanning. */
    juce::File scannerExeFile() const noexcept;

    /** Set a specific scanner exe. */
    void setScannerExe (const juce::File& exe) { _scannerExe = exe; }

private:
    friend class PluginScannerCoordinator;
    PluginManager& _manager;
    std::unique_ptr<PluginScannerCoordinator> superprocess;
    juce::ListenerList<Listener> listeners;
    juce::StringArray identifiers, failedIdentifiers;
    juce::KnownPluginList& list;
    juce::Atomic<int> cancelFlag { 0 };
    juce::File _scannerExe;

    void scanAudioFormat (const juce::String& formatName);
    bool retrieveDescriptions (const juce::String& formatName,
                               const juce::String& fileOrIdentifier,
                               juce::OwnedArray<juce::PluginDescription>& result);
};

} // namespace element

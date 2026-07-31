// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <functional>
#include <memory>

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
    void addFormat (std::unique_ptr<juce::AudioPluginFormat>);

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

    /** Cancels a running background scan and waits for it to finish.

        @param timeoutMs maximum time to wait in milliseconds
    */
    void stopScanningAudioPlugins (int timeoutMs = 5000);

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

    //==============================================================================
    /** Returns true if the plugin is hidden from the "Add Plugin" surfaces.

        Hidden state is user curation stored separately from the known plugins
        list (keyed by juce::PluginDescription::createIdentifierString()) so it
        survives plugin rescans.

        @param desc the plugin to query
        @return true if the plugin is currently hidden
    */
    bool isPluginHidden (const juce::PluginDescription& desc) const;

    /** Shows or hides a plugin. Persists immediately and broadcasts a change.

        @param desc   the plugin to update
        @param hidden true to hide the plugin from "Add Plugin" surfaces
    */
    void setPluginHidden (const juce::PluginDescription& desc, bool hidden);

    /** Returns true if the plugin is marked as a favorite.

        @param desc the plugin to query
        @return true if the plugin is a favorite
    */
    bool isPluginFavorite (const juce::PluginDescription& desc) const;

    /** Marks or unmarks a plugin as a favorite. Persists immediately and
        broadcasts a change.

        @param desc     the plugin to update
        @param favorite true to mark the plugin as a favorite
    */
    void setPluginFavorite (const juce::PluginDescription& desc, bool favorite);

    /** Returns all known plugin types with hidden ones removed.

        This is the shared filter that every "Add Plugin" surface should route
        through. The result preserves the order of getKnownPlugins().getTypes().

        @return the visible (non-hidden) plugin types
    */
    juce::Array<juce::PluginDescription> getVisiblePluginTypes() const;

    /** Returns the visible plugin types that are marked as favorites.

        @return the favorite plugin types (excludes hidden plugins)
    */
    juce::Array<juce::PluginDescription> getFavoritePluginTypes() const;

private:
    friend class PluginScanner;
    juce::PropertiesFile* props = nullptr;
    class Private;
    std::unique_ptr<Private> priv;

    friend class PluginScannerCoordinator;
    void scanFinished();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginManager)
};

class PluginScanner final : private juce::Thread {
public:
    PluginScanner (PluginManager& manager);
    ~PluginScanner();

    /** Receives scan events. Callbacks are always delivered on the
        message thread. */
    class Listener {
    public:
        Listener() {}
        virtual ~Listener() {}

        virtual void audioPluginScanFinished() {}
        virtual void audioPluginScanProgress (const float progress) { juce::ignoreUnused (progress); }
        virtual void audioPluginScanStarted (const juce::String& name) {}
    };

    static const juce::File& getWorkerPluginListFile();

    /** Starts an asynchronous scan for plugins of type on a background
        thread. Returns immediately. Does nothing if a scan is already
        in progress. */
    void scanForAudioPlugins (const juce::String& formatName);

    /** Starts an asynchronous scan for plugins of multiple types on a
        background thread. Returns immediately. Does nothing if a scan
        is already in progress. */
    void scanForAudioPlugins (const juce::StringArray& formats);

    /** Cancels the current scan operation if possible. */
    void cancel();

    /** is scanning */
    bool isScanning() const;

    /** Blocks until the scan completes, pumping the message queue when
        called from the message thread so marshalled callbacks are
        delivered.

        @param timeoutMs maximum time to wait in milliseconds
        @return true if the scan finished, false on timeout
    */
    bool waitForScanToFinish (int timeoutMs = 60000);

    /** Add a listener */
    void addListener (Listener* listener) { listeners.add (listener); }

    /** Remove a listener */
    void removeListener (Listener* listener) { listeners.remove (listener); }

    /** Returns a list of plugins that failed to load. Only valid when
        not scanning. */
    const juce::StringArray& getFailedFiles() const { return failedIdentifiers; }

    /** Returns a message describing why the last scan aborted early, or
        an empty string. Set when the scanner process repeatedly could
        not be launched or contacted. Cleared when a new scan starts. */
    juce::String getLastScanError() const;

    /** Returns the scanner exe to use for out-of-process scanning. */
    juce::File scannerExeFile() const noexcept;

    /** Set a specific scanner exe. */
    void setScannerExe (const juce::File& exe) { _scannerExe = exe; }

    /** Set the timeout used when launching the scanner process. */
    void setLaunchTimeout (int ms) { launchTimeoutMs = ms; }

    /** Set how long a single plugin may take before the scanner process
        is killed and the plugin treated as crashed. */
    void setPerPluginTimeout (int ms) { perPluginTimeoutMs = ms; }

    /** Set how many consecutive scanner-process failures abort the scan. */
    void setMaxConsecutiveFailures (int n) { maxConsecutiveFailures = n; }

private:
    friend class PluginScannerCoordinator;

    enum class ScanResult {
        ok,          ///> The worker responded. The result may be empty.
        crashed,     ///> The worker crashed or hung loading this plugin.
        unavailable, ///> The worker could not be launched or contacted.
        cancelled    ///> The scan was cancelled.
    };

    PluginManager& _manager;
    std::shared_ptr<PluginScannerCoordinator> superprocess;
    juce::ListenerList<Listener> listeners;
    juce::StringArray failedIdentifiers;
    juce::KnownPluginList& list;
    juce::Atomic<int> cancelFlag { 0 };
    juce::File _scannerExe;
    juce::StringArray formatsToScan;
    std::atomic<bool> scanning { false };
    int launchTimeoutMs;
    int perPluginTimeoutMs { 90000 };
    int maxConsecutiveFailures { 3 };
    int consecutiveFailures { 0 };
    bool abortedByFailure { false };
    juce::String lastScanError;
    juce::CriticalSection stateLock;

    void run() override;
    bool shouldAbort() const noexcept;
    void resetWorker (bool alsoKill);
    void notifyOnMessageThread (std::function<void (PluginScanner&)> fn);
    void scanAudioFormat (const juce::String& formatName);
    ScanResult retrieveDescriptions (const juce::String& formatName,
                                     const juce::String& fileOrIdentifier,
                                     juce::OwnedArray<juce::PluginDescription>& result);

    JUCE_DECLARE_WEAK_REFERENCEABLE (PluginScanner)
};

} // namespace element

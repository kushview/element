// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/core.hpp>
#include <element/juce/data_structures.hpp>
#include <element/juce/gui_basics.hpp>

namespace element {

class Context;

struct MidiPanicParams;

class Settings : public juce::ApplicationProperties,
                 public juce::ChangeBroadcaster {
public:
    Settings();
    ~Settings();

    static const char* checkForUpdatesKey;
    static const char* pluginListKey;
    static const char* pluginFormatsKey;
    static const char* pluginWindowOnTopDefault;
    static const char* lastPluginScanPathPrefix;
    static const char* scanForPluginsOnStartKey;
    static const char* showPluginWindowsKey;
    static const char* openLastUsedSessionKey;
    static const char* askToSaveSessionKey;
    static const char* defaultNewSessionFile;
    static const char* generateMidiClockKey;
    static const char* sendMidiClockToInputKey;
    static const char* hidePluginWindowsWhenFocusLostKey;
    static const char* lastGraphKey;
    static const char* lastSessionKey;
    static const char* legacyInterfaceKey;
    static const char* midiEngineKey;
    static const char* oscHostPortKey;
    static const char* oscHostEnabledKey;
    static const char* systrayKey;
    static const char* midiOutLatencyKey;
    static const char* desktopScaleKey;
    static const char* mainContentTypeKey;
    static const char* pluginListHeaderKey;
    static const char* devicesKey;
    static const char* keymappingsKey;
    static const char* clockSourceKey;
    static const char* updateChannelKey;
    static const char* updateKeyTypeKey;
    static const char* updateKeyKey;
    static const char* updateKeyUserKey;
    static const char* transportStartStopContinue;

    bool getBool (std::string_view key, bool fallback = false) const noexcept;

    void set (std::string_view key, const juce::var& value);

    std::unique_ptr<juce::XmlElement> getLastGraph() const;
    void setLastGraph (const juce::ValueTree& data);

    /** Returns true if updates shoul be checked for on launch */
    bool checkForUpdates() const;

    /** Set if should check updates on start */
    void setCheckForUpdates (const bool shouldCheck);

    /** Returns true if plugins should be scanned on startup */
    bool scanForPluginsOnStartup() const;

    /** Set if plugins should be scanned during startup */
    void setScanForPluginsOnStartup (const bool shouldScan);

    /** True if plugin windows should be made visible when added to a graph */
    bool showPluginWindowsWhenAdded() const;
    void setShowPluginWindowsWhenAdded (const bool);

    /** True if the last used session should be opened on launch.
        If running as Solo/Lite, then this returns true if the last
        Graph should be opened on launch instead of a Session
     */
    bool openLastUsedSession() const;
    void setOpenLastUsedSession (const bool);

    /** True if plugin windows are on top by default */
    bool pluginWindowsOnTop() const;
    void setPluginWindowsOnTop (const bool);

    /** True if the user should be prompted to save when exiting the app */
    bool askToSaveSession();
    void setAskToSaveSession (const bool);

    const juce::File getDefaultNewSessionFile() const;
    void setDefaultNewSessionFile (const juce::File&);

    void addItemsToMenu (Context&, juce::PopupMenu&);
    bool performMenuResult (Context&, const int result);

    void setGenerateMidiClock (const bool);
    bool generateMidiClock() const;

    void setSendMidiClockToInput (const bool);
    bool sendMidiClockToInput() const;

    void setHidePluginWindowsWhenFocusLost (const bool);
    bool hidePluginWindowsWhenFocusLost() const;

    void setUseLegacyInterface (const bool);
    bool useLegacyInterface() const;

    bool isOscHostEnabled() const;
    void setOscHostEnabled (bool);
    int getOscHostPort() const;
    void setOscHostPort (int);

    bool isSystrayEnabled() const;
    void setSystrayEnabled (bool);

    double getMidiOutLatency() const;
    void setMidiOutLatency (double latencyMs);

    double getDesktopScale() const;
    void setDesktopScale (double);

    juce::String getMainContentType() const;
    void setMainContentType (const juce::String&);

    juce::String getClockSource() const;
    void setClockSource (const juce::String&);

    /** Returns the update Key type to use when checking. */
    juce::String getUpdateKeyType() const;

    /** Set the key type. Possible values are patreon, element-v1, or 
        membership.
     */
    void setUpdateKeyType (const String& slug);

    /** Returns the update key user. */
    juce::String getUpdateKeyUser() const;

    /** Set the update key user. */
    void setUpdateKeyUser (const String& user);

    /** Returns the update Key to use when checking. */
    juce::String getUpdateKey() const;

    /** Set the user's update key text. */
    void setUpdateKey (const String& key);

    /** Returns the update channel. */
    juce::String getUpdateChannel() const;

    /** Change the update channgel. stable or nightly. */
    void setUpdateChannel (const String& key);

    /** Set global midi panic settings. */
    void setMidiPanicParams (MidiPanicParams);

    /** Get global midi panic settings. */
    MidiPanicParams getMidiPanicParams() const;

    void setTransportRespondToStartStopContinue (bool shouldRespond);
    bool transportRespondToStartStopContinue() const;

private:
    juce::PropertiesFile* getProps() const;
};

} // namespace element

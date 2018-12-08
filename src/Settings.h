/*
    Settings.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"

namespace Element {

class Globals;

class Settings :  public ApplicationProperties
{
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

    XmlElement* getLastGraph() const;
    void setLastGraph (const ValueTree& data);

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

    /** True if the last used session should be opened on launch */
    bool openLastUsedSession() const;
    void setOpenLastUsedSession (const bool);

    /** True if plugin windows are on top by default */
    bool pluginWindowsOnTop() const;
    void setPluginWindowsOnTop (const bool);

    bool askToSaveSession();
    void setAskToSaveSession (const bool);

    const File getDefaultNewSessionFile() const;
    void setDefaultNewSessionFile (const File&);

    void addItemsToMenu (Globals&, PopupMenu&);
    bool performMenuResult (Globals&, const int result);

    void setGenerateMidiClock (const bool);
    bool generateMidiClock() const;

    void setSendMidiClockToInput (const bool);
    bool sendMidiClockToInput() const;

private:
    PropertiesFile* getProps() const;
};

}

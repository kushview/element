/*
    Globals.h - This file is part of Element
    Copyright (C) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#ifndef EL_GLOBALS_H
#define EL_GLOBALS_H

#include "ElementApp.h"
#include "engine/AudioEngine.h"
#include "URIs.h"
#include "WorldBase.h"

namespace Element {

class CommandManager;
class DeviceManager;
class MediaManager;
class PluginManager;
class Session;
class Settings;
class Writer;

struct CommandLine
{
    explicit CommandLine (const String& cli = String::empty);
    bool fullScreen;
    int port;
};

class Globals : public WorldBase
{
public:
    explicit Globals (const String& commandLine = String::empty);
    ~Globals();

    const CommandLine cli;

    AudioEnginePtr getAudioEngine() const;
    CommandManager& getCommandManager();
    DeviceManager& getDeviceManager();
    PluginManager& getPluginManager();
    Settings& getSettings();
    MediaManager& getMediaManager();
    
    PluginManager& plugins();
    Settings& settings();
    SymbolMap& symbols();
    MediaManager& media();
    Session& session();

    const String& getAppName() const { return appName; }
    void setEngine (EnginePtr engine);

    JUCE_DEPRECATED(DeviceManager& devices());
    JUCE_DEPRECATED(AudioEnginePtr engine() const);
private:
    String appName;
    friend class Application;
    class Impl;
    ScopedPointer<Impl> impl;
};

}

#endif // EL_GLOBALS_H

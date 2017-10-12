/*
    Globals.h - This file is part of Element
    Copyright (C) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "engine/AudioEngine.h"
#include "session/Session.h"
#include "URIs.h"
#include "WorldBase.h"

namespace Element {

class CommandManager;
class DeviceManager;
class MediaManager;
class PluginManager;
class Settings;
class UnlockStatus;
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
    UnlockStatus& getUnlockStatus();
    SymbolMap& getSymbolMap();
    SessionPtr getSession();

    const String& getAppName() const { return appName; }
    void setEngine (EnginePtr engine);

private:
    String appName;
    friend class Application;
    class Impl;
    ScopedPointer<Impl> impl;
};

}

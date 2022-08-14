/*
    Globals.h - This file is part of Element
    Copyright (C) 2016-2017 Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "ElementApp.h"
#include "engine/audioengine.hpp"
#include "engine/mappingengine.hpp"
#include "engine/midiengine.hpp"
#include "session/session.hpp"

namespace element { class Context; }

namespace Element {
using Context = element::Context;
class CommandManager;
class DeviceManager;
class ScriptingEngine;
class Log;
class PluginManager;
class PresetManager;
class Settings;
class Writer;

struct CommandLine
{
    explicit CommandLine (const String& cli = String());
    bool fullScreen;
    int port;

    const String commandLine;
};

class Globals
{
public:
    explicit Globals (const String& commandLine = String());
    virtual ~Globals();

    const CommandLine cli;

    Context& getContext();
    AudioEnginePtr getAudioEngine() const;
    CommandManager& getCommandManager();
    DeviceManager& getDeviceManager();
    Log& getLog();
    MappingEngine& getMappingEngine();
    MidiEngine& getMidiEngine();
    PluginManager& getPluginManager();
    PresetManager& getPresetManager();
    Settings& getSettings();
    ScriptingEngine& getScriptingEngine();
    SessionPtr getSession();

    const String& getAppName() const { return appName; }
    void setEngine (AudioEnginePtr engine);

private:
    friend class Application;
    class Impl;
    String appName;
    std::unique_ptr<Impl> impl;
};

} // namespace Element

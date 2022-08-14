/*
    Globals.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

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

#include <element/context.hpp>

#include "ElementApp.h"
#include "engine/internalformat.hpp"
#include "scripting/scriptingengine.hpp"
#include "session/devicemanager.hpp"
#include "session/pluginmanager.hpp"
#include "session/presetmanager.hpp"
#include "session/session.hpp"
#include "settings.hpp"
#include "session/commandmanager.hpp"
#include "globals.hpp"
#include "log.hpp"

namespace Element {

static void buildCommandLine (CommandLine& cli, const String& c)
{
    cli.fullScreen = c.contains ("--full-screen");
    const var port = c.fromFirstOccurrenceOf ("--port=", false, false)
                         .upToFirstOccurrenceOf (" ", false, false);
    if (port.isInt() || port.isInt64())
        cli.port = (int) port;
}

CommandLine::CommandLine (const String& c)
    : fullScreen (false),
      port (3123),
      commandLine (c)
{
    if (c.isNotEmpty())
        buildCommandLine (*this, commandLine);
}

class Globals::Impl
{
public:
    Impl (Globals& g)
        : owner (g) {}

    ~Impl() {}

    Globals& owner;

    AudioEnginePtr engine;
    SessionPtr session;

    std::unique_ptr<Context> context;
    std::unique_ptr<CommandManager> commands;
    std::unique_ptr<DeviceManager> devices;
    std::unique_ptr<PluginManager> plugins;
    std::unique_ptr<Settings> settings;
    std::unique_ptr<MappingEngine> mapping;
    std::unique_ptr<PresetManager> presets;
    std::unique_ptr<MidiEngine> midi;
    std::unique_ptr<ScriptingEngine> lua;
    std::unique_ptr<Log> log;

private:
    friend class Globals;

    void init()
    {
        context.reset (new Context());
        log.reset (new Log());
        plugins.reset (new PluginManager());
        devices.reset (new DeviceManager());
        settings.reset (new Settings());
        commands.reset (new CommandManager());
        mapping.reset (new MappingEngine());
        midi.reset (new MidiEngine());
        presets.reset (new PresetManager());
        session = new Session();

        lua.reset (new ScriptingEngine());
        lua->initialize (owner);
    }

    void freeAll()
    {
        commands = nullptr;
        plugins = nullptr;
        settings = nullptr;
        engine = nullptr;
        session = nullptr;
        devices = nullptr;
        midi = nullptr;
        presets = nullptr;
        lua = nullptr;
        log = nullptr;
        context = nullptr;
    }
};

Globals::Globals (const String& _cli)
    : cli (_cli)
{
    appName = "Element";
    impl.reset (new Impl (*this));
    impl->init();
}

Globals::~Globals()
{
    impl->freeAll();
}

Context& Globals::getContext()
{
    jassert (impl && impl->context);
    return *impl->context;
}

CommandManager& Globals::getCommandManager()
{
    jassert (impl && impl->commands);
    return *impl->commands;
}

DeviceManager& Globals::getDeviceManager()
{
    jassert (impl->devices != nullptr);
    return *impl->devices;
}

Log& Globals::getLog()
{
    jassert (impl != nullptr && impl->log != nullptr);
    return *impl->log;
}

MappingEngine& Globals::getMappingEngine()
{
    jassert (impl != nullptr && impl->mapping != nullptr);
    return *impl->mapping;
}

MidiEngine& Globals::getMidiEngine()
{
    jassert (impl != nullptr && impl->midi != nullptr);
    return *impl->midi;
}

ScriptingEngine& Globals::getScriptingEngine()
{
    jassert (impl->lua != nullptr);
    return *impl->lua;
}

AudioEnginePtr Globals::getAudioEngine() const { return impl->engine; }

PluginManager& Globals::getPluginManager()
{
    jassert (impl->plugins != nullptr);
    return *impl->plugins;
}

PresetManager& Globals::getPresetManager()
{
    jassert (impl->presets != nullptr);
    return *impl->presets;
}

Settings& Globals::getSettings()
{
    jassert (impl->settings != nullptr);
    return *impl->settings;
}

SessionPtr Globals::getSession()
{
    return (impl) ? impl->session : nullptr;
}

void Globals::setEngine (AudioEnginePtr engine)
{
    if (impl->engine)
        impl->engine->deactivate();
    impl->engine = engine;

    getDeviceManager().attach (engine);
}

} // namespace Element

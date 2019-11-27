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

#include "ElementApp.h"
#include "engine/InternalFormat.h"
#include "scripting/LuaEngine.h"
#include "session/DeviceManager.h"
#include "session/MediaManager.h"
#include "session/PluginManager.h"
#include "session/Presets.h"
#include "session/Session.h"
#include "Settings.h"
#include "URIs.h"
#include "session/CommandManager.h"
#include "Globals.h"

namespace Element {

static void buildCommandLine (CommandLine& cli, const String& c)
{
    cli.fullScreen = c.contains ("--full-screen");
    const var port = c.fromFirstOccurrenceOf("--port=", false, false)
                      .upToFirstOccurrenceOf(" ", false, false);
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
        : owner (g) { }
    
    ~Impl() { }

    Globals& owner;
    AudioEnginePtr                engine;
    SessionPtr                    session;
    
    ScopedPointer<CommandManager> commands;
    ScopedPointer<DeviceManager>  devices;
    ScopedPointer<MediaManager>   media;
    ScopedPointer<PluginManager>  plugins;
    ScopedPointer<Settings>       settings;
    std::unique_ptr<MappingEngine> mapping;
    std::unique_ptr<PresetCollection> presets;
    std::unique_ptr<MidiEngine>   midi;
    std::unique_ptr<LuaEngine>    lua;
   
private:
    friend class Globals;
    
    void init()
    {
        plugins  = new PluginManager();
        devices  = new DeviceManager();
        media    = new MediaManager();
        settings = new Settings();
        commands = new CommandManager();
        session  = new Session();
        mapping.reset (new MappingEngine());
        midi.reset (new MidiEngine());
        presets.reset (new PresetCollection());
        lua.reset (new LuaEngine());
        lua->setWorld (owner);
    }
    
    void freeAll()
    {
        commands = nullptr;
        plugins  = nullptr;
        settings = nullptr;
        engine   = nullptr;
        session  = nullptr;
        media    = nullptr;
        devices  = nullptr;
        midi     = nullptr;
        presets  = nullptr;
        lua      = nullptr;
    }
};

Globals::Globals (const String& _cli)
    : WorldBase (this),
      cli (_cli)
{
    appName = "Element";
    impl = new Impl (*this);
    impl->init();
}

Globals::~Globals()
{
    impl->freeAll();
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

MediaManager& Globals::getMediaManager()
{
    jassert (impl->media != nullptr);
    return *impl->media;
}

LuaEngine& Globals::getLuaEngine()
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

PresetCollection& Globals::getPresetCollection() 
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

}

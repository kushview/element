/*
    context.cpp - This file is part of Element
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

#include "engine/internalformat.hpp"
#include "scripting.hpp"
#include "session/devicemanager.hpp"
#include "session/pluginmanager.hpp"
#include "session/presetmanager.hpp"
#include "session/session.hpp"
#include "session/commandmanager.hpp"

#include "context.hpp"
#include "log.hpp"
#include "module.hpp"
#include "settings.hpp"

namespace element {

class Context::Impl
{
public:
    Impl (Context& g)
        : owner (g) {}

    ~Impl() {}

    Context& owner;

    AudioEnginePtr engine;
    SessionPtr session;

    std::unique_ptr<CommandManager> commands;
    std::unique_ptr<DeviceManager> devices;
    std::unique_ptr<PluginManager> plugins;
    std::unique_ptr<Settings> settings;
    std::unique_ptr<MappingEngine> mapping;
    std::unique_ptr<PresetManager> presets;
    std::unique_ptr<MidiEngine> midi;
    std::unique_ptr<ScriptingEngine> lua;
    std::unique_ptr<Log> log;
    std::unique_ptr<Modules> modules;

private:
    friend class Context;

    void init()
    {
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
    }
};

Context::Context (const String& _cli)
{
    appName = "Element";
    impl.reset (new Impl (*this));
    impl->init();
}

Context::~Context()
{
    impl->freeAll();
}

//=============================================================================
CommandManager& Context::getCommandManager()
{
    jassert (impl && impl->commands);
    return *impl->commands;
}

DeviceManager& Context::getDeviceManager()
{
    jassert (impl->devices != nullptr);
    return *impl->devices;
}

Log& Context::getLog()
{
    jassert (impl != nullptr && impl->log != nullptr);
    return *impl->log;
}

MappingEngine& Context::getMappingEngine()
{
    jassert (impl != nullptr && impl->mapping != nullptr);
    return *impl->mapping;
}

MidiEngine& Context::getMidiEngine()
{
    jassert (impl != nullptr && impl->midi != nullptr);
    return *impl->midi;
}

ScriptingEngine& Context::getScriptingEngine()
{
    jassert (impl->lua != nullptr);
    return *impl->lua;
}

AudioEnginePtr Context::getAudioEngine() const { return impl->engine; }

PluginManager& Context::getPluginManager()
{
    jassert (impl->plugins != nullptr);
    return *impl->plugins;
}

PresetManager& Context::getPresetManager()
{
    jassert (impl->presets != nullptr);
    return *impl->presets;
}

Settings& Context::getSettings()
{
    jassert (impl->settings != nullptr);
    return *impl->settings;
}

SessionPtr Context::getSession()
{
    return (impl) ? impl->session : nullptr;
}

void Context::setEngine (AudioEnginePtr engine)
{
    if (impl->engine)
        impl->engine->deactivate();
    impl->engine = engine;

    getDeviceManager().attach (engine);
}

void Context::openModule (const std::string& ID)
{
    if (impl->modules->contains (ID))
        return;

    auto it = impl->modules->discovered.find (ID);
    if (it == impl->modules->discovered.end()) {
        std::clog << "module not found: " << ID << std::endl;
        return;
    }

    if (auto mod = std::make_unique<Module> (it->second, *this, *impl->lua)) {
        if (mod->open()) {
            std::clog << "module opened: " << mod->name() << std::endl;
            impl->modules->add (std::move (mod));
        } else {
            std::clog << "could not open module: " << mod->name() << std::endl;
        }
    }
}

void Context::loadModules()
{
    std::vector<const elFeature*> features;

    for (const auto& mod : *impl->modules) {
        for (const auto& e : mod->public_extensions()) {
            auto f = (elFeature*) std::malloc (sizeof (elFeature));
            features.push_back (f);
            f->ID = strdup (e.first.c_str());
            f->data = (void*) e.second;
        }
    }

    auto ctxfeature = (elFeature*) malloc (sizeof (elFeature));
    ctxfeature->ID = strdup ("el.Context");
    ctxfeature->data = this;
    features.push_back (ctxfeature);
    features.push_back ((elFeature*) nullptr);
    elFeatures fptr = &features.front();

    for (const auto& mod : *impl->modules) {
        if (! mod->loaded())
            mod->load (fptr);
    }

    for (auto f : features) {
        if (nullptr == f)
            continue;
        std::free ((void*) f->ID);
        std::free ((void*) f);
    }
    features.clear();
}

void Context::addModulePath (const std::string& path)
{
    auto& sp = impl->modules->searchpath;
    sp.add (path);
}

void Context::discoverModules()
{
    impl->modules->discover();
}

} // namespace element

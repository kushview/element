// Copyright 2022-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/devices.hpp>
#include <element/session.hpp>
#include <element/settings.hpp>
#include <element/plugins.hpp>
#include <element/services.hpp>
#include <element/symbolmap.hpp>
#include <element/context.hpp>
#include <element/lv2.hpp>

#include "engine/audioprocessorfactory.hpp"
#include "engine/internalformat.hpp"
#include "engine/mappingengine.hpp"
#include "engine/midiengine.hpp"
#include "engine/clapprovider.hpp"
#include "presetmanager.hpp"

#include "appinfo.hpp"
#include "log.hpp"
#include "module.hpp"
#include "scripting.hpp"

namespace element {

namespace detail {
inline static void setupChildProcessHandlers (RunMode mode)
{
    if (mode == RunMode::Plugin)
        return;
#if JUCE_LINUX
    // JUCE allows zombie processes... this prevents them as a workaround.
    static bool _init = false;
    if (! _init)
    {
        _init = true;
        signal (SIGCHLD, SIG_IGN);
    }
#endif
}
} // namespace detail

class Context::Impl
{
public:
    Impl (Context& g, RunMode m)
        : owner (g),
          mode (m) {}

    ~Impl() {}

    Context& owner;
    RunMode mode;

    std::unique_ptr<Services> services;

    AudioEnginePtr engine;
    SessionPtr session;

    std::unique_ptr<SymbolMap> symbols;
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
        symbols.reset (new SymbolMap());
        log.reset (new Log());
        devices.reset (new DeviceManager());
        settings.reset (new Settings());
        mapping.reset (new MappingEngine());
        midi.reset (new MidiEngine());
        presets.reset (new PresetManager());
        session = new Session();

        lua.reset (new ScriptingEngine());
        lua->initialize (owner);

        owner.setEngine (new AudioEngine (owner, mode));
        services = std::make_unique<Services> (owner, mode);

        plugins.reset (new PluginManager());
        auto& nf = plugins->getNodeFactory();
        nf.add (new InternalNodes (owner));
        nf.add (new AudioProcessorFactory (owner));
        nf.add (new LV2NodeProvider (*symbols));
        nf.add (new CLAPProvider());
        plugins->addDefaultFormats();
    }

    void freeAll()
    {
        services = nullptr;
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

Context::Context (RunMode mode, const String& _cli)
{
    impl.reset (new Impl (*this, mode));
    detail::setupChildProcessHandlers (mode);
    impl->init();
}

Context::~Context()
{
    impl->freeAll();
}

//=============================================================================
Services& Context::services()
{
    jassert (impl && impl->services);
    return *impl->services;
}

DeviceManager& Context::devices()
{
    jassert (impl->devices != nullptr);
    return *impl->devices;
}

Log& Context::logger()
{
    jassert (impl != nullptr && impl->log != nullptr);
    return *impl->log;
}

MappingEngine& Context::mapping()
{
    jassert (impl != nullptr && impl->mapping != nullptr);
    return *impl->mapping;
}

MidiEngine& Context::midi()
{
    jassert (impl != nullptr && impl->midi != nullptr);
    return *impl->midi;
}

ScriptingEngine& Context::scripting()
{
    jassert (impl->lua != nullptr);
    return *impl->lua;
}

AudioEnginePtr Context::audio() const { return impl->engine; }

PluginManager& Context::plugins()
{
    jassert (impl->plugins != nullptr);
    return *impl->plugins;
}

PresetManager& Context::presets()
{
    jassert (impl->presets != nullptr);
    return *impl->presets;
}

Settings& Context::settings()
{
    jassert (impl->settings != nullptr);
    return *impl->settings;
}

SessionPtr Context::session()
{
    return (impl) ? impl->session : nullptr;
}

void Context::setEngine (AudioEnginePtr engine)
{
    if (impl->engine)
        impl->engine->deactivate();
    impl->engine = engine;

    devices().attach (engine);
}

void Context::openModule (const std::string& ID)
{
    if (impl->modules->contains (ID))
        return;

    auto it = impl->modules->discovered.find (ID);
    if (it == impl->modules->discovered.end())
    {
        std::clog << "module not found: " << ID << std::endl;
        return;
    }

    if (auto mod = std::make_unique<Module> (it->second, *this, *impl->lua))
    {
        if (mod->open())
        {
            std::clog << "module opened: " << mod->name() << std::endl;
            impl->modules->add (std::move (mod));
        }
        else
        {
            std::clog << "could not open module: " << mod->name() << std::endl;
        }
    }
}

void Context::loadModules()
{
    std::vector<const elFeature*> features;

    for (const auto& mod : *impl->modules)
    {
        for (const auto& e : mod->public_extensions())
        {
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

    for (const auto& mod : *impl->modules)
    {
        if (! mod->loaded())
            mod->load (fptr);
    }

    for (auto f : features)
    {
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

void Context::discoverModules() { impl->modules->discover(); }
SymbolMap& Context::symbols() { return *impl->symbols; }

} // namespace element

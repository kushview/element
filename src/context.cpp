// Copyright 2022-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/devices.hpp>
#include <element/session.hpp>
#include <element/settings.hpp>
#include <element/plugins.hpp>
#include <element/services.hpp>
#include <element/context.hpp>

#include "engine/audioprocessorfactory.hpp"
#include "engine/internalformat.hpp"
#include "engine/mappingengine.hpp"
#include "engine/midiengine.hpp"
#include "engine/clapprovider.hpp"
#include "presetmanager.hpp"

#include "appinfo.hpp"
#include "log.hpp"
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

    std::unique_ptr<DeviceManager> devices;
    std::unique_ptr<PluginManager> plugins;
    std::unique_ptr<Settings> settings;
    std::unique_ptr<MappingEngine> mapping;
    std::unique_ptr<PresetManager> presets;
    std::unique_ptr<MidiEngine> midi;
    std::unique_ptr<ScriptingEngine> lua;
    std::unique_ptr<Log> log;

private:
    friend class Context;

    void init()
    {
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

} // namespace element

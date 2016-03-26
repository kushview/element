/*
    Globals.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "element/Juce.h"
#include "session/Session.h"
#include "CommandManager.h"
#include "Globals.h"
#include "MediaManager.h"
#include "Settings.h"
#include "URIs.h"

namespace Element {

static void buildCommandLine (CommandLine& cli, const String& c) {
    cli.fullScreen = c.contains ("--full-screen");

    const var port = c.fromFirstOccurrenceOf("--port=", false, false)
                      .upToFirstOccurrenceOf(" ", false, false);
    if (port != var::null)
        cli.port = (int) port;
}

CommandLine::CommandLine (const String& c)
    : fullScreen (false),
      port (3123)
{
    if (c.isNotEmpty())
        buildCommandLine (*this, c);
}

class Globals::Impl
{
public:
    Impl (Globals& g)
        : owner (g)
    {
       #if !ELEMENT_LV2_PLUGIN_HOST
        symbols  = new SymbolMap();
       #endif
        plugins  = new PluginManager();
        devices  = new DeviceManager();
        media    = new MediaManager();
        settings = new Settings();
        commands = new CommandManager();
    }

    void freeAll()
    {
        commands = nullptr;
        session  = nullptr;
        devices  = nullptr;
        media    = nullptr;
        plugins  = nullptr;
        settings = nullptr;
        engine   = nullptr;
    }

    ~Impl() { }

    Globals& owner;
    AudioEnginePtr                engine;
    ScopedPointer<CommandManager> commands;
    ScopedPointer<DeviceManager>  devices;
    ScopedPointer<MediaManager>   media;
    ScopedPointer<PluginManager>  plugins;
    ScopedPointer<Settings>       settings;
    ScopedPointer<Session>        session;
   #if !ELEMENT_LV2_PLUGIN_HOST
    ScopedPointer<SymbolMap>      symbols;
   #endif
};

Globals::Globals (const String& _cli)
    : WorldBase (this),
      cli (_cli)
{
    appName = "Element";
    impl = new Impl (*this);
}

Globals::~Globals()
{
    impl->freeAll();
}

CommandManager& Globals::getCommands() { assert(impl->commands); return *impl->commands; }

DeviceManager& Globals::devices()
{
    assert (impl->devices != nullptr);
    return *impl->devices;
}

MediaManager& Globals::media()
{
    assert (impl->media != nullptr);
    return *impl->media;
}

AudioEnginePtr Globals::engine() const { return impl->engine; }

PluginManager& Globals::plugins()
{
    assert (impl->plugins != nullptr);
    return *impl->plugins;
}

Settings& Globals::settings()
{
    assert (impl->settings != nullptr);
    return *impl->settings;
}

SymbolMap& Globals::symbols()
{
   #if ELEMENT_LV2_PLUGIN_HOST
    auto* fmt = impl->plugins->format<LV2PluginFormat>();
    jassert(fmt);
    return fmt->getSymbolMap();
   #else
    assert(impl->symbols);
    return *impl->symbols;
   #endif
}

Session& Globals::session()
{
    assert (impl->session != nullptr);
    return *impl->session;
}

void Globals::setEngine (EnginePtr engine)
{
    impl->engine = engine;

    if (impl->session == nullptr) {
        impl->session = new Session (*this);
    }

    devices().attach (engine);
}

}

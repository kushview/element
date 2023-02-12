/*
    context.h - This file is part of Element
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

#include <element/juce/core.hpp>

#include <element/audioengine.hpp>
#include <element/session.hpp>

namespace element {

class ServiceManager;
class CommandManager;
class DeviceManager;
class MappingEngine;
class MidiEngine;
class ScriptingEngine;
class Log;
class PluginManager;
class PresetManager;
class Settings;

class Context
{
public:
    explicit Context (const juce::String& commandLine = juce::String());
    virtual ~Context();

    ServiceManager& getServices();

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

    const std::string& getAppName() const { return appName; }
    void setEngine (AudioEnginePtr engine);

    //=========================================================================
    void openModule (const std::string& path);
    void loadModules();
    void addModulePath (const std::string& path);
    void discoverModules();

    //===
    void testLaunch();
    
private:
    friend class Application;
    std::string appName;
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace element

/*
    This file is part of Element
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

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

#include "gui/Workspace.h"
#include "session/DeviceManager.h"
#include "Globals.h"
#include "Settings.h"

namespace Element {

const char* Settings::checkForUpdatesKey        = "checkForUpdates";
const char* Settings::pluginFormatsKey          = "pluginFormatsKey";
const char* Settings::pluginWindowOnTopDefault  = "pluginWindowOnTopDefault";
const char* Settings::scanForPluginsOnStartKey  = "scanForPluginsOnStart";
const char* Settings::showPluginWindowsKey      = "showPluginWindows";
const char* Settings::openLastUsedSessionKey    = "openLastUsedSession";
const char* Settings::askToSaveSessionKey       = "askToSaveSession";
const char* Settings::defaultNewSessionFile     = "defaultNewSessionFile";
const char* Settings::generateMidiClockKey      = "generateMidiClockKey";
const char* Settings::sendMidiClockToInputKey   = "sendMidiClockToInputKey";
const char* Settings::hidePluginWindowsWhenFocusLostKey = "hidePluginWindowsWhenFocusLost";
const char* Settings::lastGraphKey              = "lastGraph";
const char* Settings::legacyInterfaceKey        = "legacyInterface";
const char* Settings::workspaceKey              = "workspace";
const char* Settings::midiEngineKey             = "midiEngine";

enum OptionsMenuItemId
{
    CheckForUpdatesOnStart = 1000000,
    ScanFormPluginsOnStart,
    AutomaticallyShowPluginWindows,
    HidePluginWindowsWhenFocusLost,
    PluginWindowsOnTop,
    OpenLastUsedSession,
    AskToSaveSessions,

    MidiInputDevice = 2000000,
    MidiOutputDevice = 3000000,
    AudioInputDevice = 4000000,
    AudioOutputDevice = 5000000,
    SampleRate = 6000000,
    BufferSize = 7000000
};

static bool settingResultIsFor (const int result, const int optionMenuId) {
    return (result >= optionMenuId && result < optionMenuId + 1000000);
}

#if JUCE_32BIT
 #if JUCE_MAC
  const char* Settings::lastPluginScanPathPrefix = "pluginScanPath32_";
  const char* Settings::pluginListKey            = "pluginList32";
 
 #else
  const char* Settings::lastPluginScanPathPrefix = "pluginScanPath_";   // TODO: migrate this
  const char* Settings::pluginListKey            = "plugin-list";       // TODO: migrate this
 #endif

#else // 64bit keys
 #if JUCE_MAC
  const char* Settings::lastPluginScanPathPrefix = "pluginScanPath_";   // TODO: migrate this
  const char* Settings::pluginListKey            = "plugin-list";       // TODO: migrate this
 
 #else
  const char* Settings::lastPluginScanPathPrefix = "pluginScanPath64_";
  const char* Settings::pluginListKey            = "pluginList64";
 #endif
#endif
    
Settings::Settings()
{
    PropertiesFile::Options opts;
    opts.applicationName     = "Element";
    opts.filenameSuffix      = "conf";
    opts.osxLibrarySubFolder = "Application Support";
    opts.storageFormat       = PropertiesFile::storeAsCompressedBinary;

   #if JUCE_DEBUG
    opts.applicationName << "Debug";
    opts.storageFormat       = PropertiesFile::storeAsXML;
   #endif
    
   #if JUCE_LINUX
    opts.folderName          = ".config/Element";
   #else
    opts.folderName          = "Element";
   #endif

    setStorageParameters (opts);
}

Settings::~Settings() { }

    
bool Settings::checkForUpdates() const
{
    if (auto* props = getProps())
        return props->getBoolValue (checkForUpdatesKey, true);
    return false;
}

void Settings::setCheckForUpdates (const bool shouldCheck)
{
    if (shouldCheck == checkForUpdates())
        return;
    if (auto* p = getProps())
        p->setValue (checkForUpdatesKey, shouldCheck);
}
    
PropertiesFile* Settings::getProps() const
{
    return (const_cast<Settings*> (this))->getUserSettings();
}

    
XmlElement* Settings::getLastGraph() const
{
    if (auto* p = getProps())
        return p->getXmlValue ("lastGraph").release();
    return nullptr;
}

void Settings::setLastGraph (const ValueTree& data)
{
    jassert (data.hasType (Tags::node));
    if (! data.hasType (Tags::node))
        return;
    if (auto* p = getProps())
        if (auto xml = data.createXml())
            p->setValue ("lastGraph", xml.get());
}
    

bool Settings::scanForPluginsOnStartup() const
{
    if (auto* p = getProps())
        return p->getBoolValue (scanForPluginsOnStartKey, false);
    return false;
}
    
void Settings::setScanForPluginsOnStartup (const bool shouldScan)
{
    if (shouldScan == scanForPluginsOnStartup())
        return;
    if (auto* p = getProps())
        p->setValue (scanForPluginsOnStartKey, shouldScan);
}

bool Settings::showPluginWindowsWhenAdded() const
{
    if (auto* p = getProps())
        return p->getBoolValue (showPluginWindowsKey, true);
    return true;
}

void Settings::setShowPluginWindowsWhenAdded (const bool shouldShow)
{
    if (shouldShow == showPluginWindowsWhenAdded())
        return;
    if (auto* p = getProps())
        p->setValue (showPluginWindowsKey, shouldShow);
}

bool Settings::openLastUsedSession() const
{
    if (auto* p = getProps())
        return p->getBoolValue (openLastUsedSessionKey, true);
    return true;
}

void Settings::setOpenLastUsedSession (const bool shouldOpen)
{
    if (shouldOpen == openLastUsedSession())
        return;
    if (auto* p = getProps())
        p->setValue (openLastUsedSessionKey, shouldOpen);
}

void Settings::setGenerateMidiClock (const bool generate)
{
    if (auto* p = getProps())
        return p->setValue (generateMidiClockKey, generate);
}

bool Settings::generateMidiClock() const
{
    if (auto* p = getProps())
        return p->getBoolValue (generateMidiClockKey, false);
    return false;
}

bool Settings::pluginWindowsOnTop() const
{
    if (auto* p = getProps())
        return p->getBoolValue (pluginWindowOnTopDefault, true);
    return false;
}

bool Settings::askToSaveSession()
{
    if (auto* props = getProps())
        return props->getBoolValue (askToSaveSessionKey, true);
    return false;
}

void Settings::setAskToSaveSession (const bool value)
{
    if (auto* props = getProps())
        props->setValue (askToSaveSessionKey, value);
}


bool Settings::sendMidiClockToInput() const
{
    if (auto* props = getProps())
        return props->getBoolValue (sendMidiClockToInputKey, false);
    return false;
}

void Settings::setSendMidiClockToInput (const bool value)
{
    if (auto* props = getProps())
        props->setValue (sendMidiClockToInputKey, value);
}

void Settings::setPluginWindowsOnTop (const bool onTop)
{
    if (onTop == pluginWindowsOnTop())
        return;
    if (auto* p = getProps())
        p->setValue (pluginWindowOnTopDefault, onTop);
}

const File Settings::getDefaultNewSessionFile() const
{
    if (auto* p = getProps())
    {
        const auto value = p->getValue (defaultNewSessionFile);
        if (value.isNotEmpty() && File::isAbsolutePath (value))
            return File (value);
    }

    return File();
}

void Settings::setDefaultNewSessionFile (const File& file)
{
    if (auto* p = getProps())
        p->setValue (defaultNewSessionFile, 
            file.existsAsFile() ? file.getFullPathName() : "");
}


bool Settings::hidePluginWindowsWhenFocusLost() const
{
    if (auto* p = getProps())
        return p->getBoolValue (hidePluginWindowsWhenFocusLostKey, true);
    return true;
}

void Settings::setHidePluginWindowsWhenFocusLost (const bool hideThem)
{
    if (hideThem == hidePluginWindowsWhenFocusLost())
        return;
    if (auto* p = getProps())
        p->setValue (hidePluginWindowsWhenFocusLostKey, hideThem);
}

bool Settings::useLegacyInterface() const
{
    if (auto* p = getProps())
        return p->getBoolValue (legacyInterfaceKey, false);
    return false;
}

void Settings::setUseLegacyInterface (const bool useLegacy)
{
    if (useLegacy == useLegacyInterface())
        return;
    if (auto* p = getProps())
        p->setValue (legacyInterfaceKey, useLegacy);
}

void Settings::setWorkspace (const String& name)
{
    if (getWorkspace() == name)
        return;
    if (auto* p = getProps())
        p->setValue (workspaceKey, name);
}

String Settings::getWorkspace() const
{
    if (auto* p = getProps())
        return p->getValue (workspaceKey, EL_WORKSPACE_CLASSIC);
    return EL_WORKSPACE_CLASSIC;
}

File Settings::getWorkspaceFile() const
{
    auto name = getWorkspace();
    if (name.isNotEmpty()) name << ".elw";
    return name.isNotEmpty() ? DataPath::workspacesDir().getChildFile (name)
                             : File();
}

void Settings::addItemsToMenu (Globals& world, PopupMenu& menu)
{
    auto& devices (world.getDeviceManager());
    auto& midi (world.getMidiEngine());
    PopupMenu sub;

    sub.addItem (CheckForUpdatesOnStart, "Check Updates at Startup", 
        true, checkForUpdates());
    
    sub.addSeparator(); // plugins items
    
    sub.addItem (ScanFormPluginsOnStart, "Scan Plugins at Startup", 
        true, scanForPluginsOnStartup());
    sub.addItem (AutomaticallyShowPluginWindows, "Automatically Show Plugin Windows", 
        true, showPluginWindowsWhenAdded());
    sub.addItem (PluginWindowsOnTop, "Plugins On Top By Default", 
        true, pluginWindowsOnTop());
    sub.addItem (HidePluginWindowsWhenFocusLost, "Hide Plugin Windows When App Inactive", 
        true, hidePluginWindowsWhenFocusLost());
    
    sub.addSeparator(); // session items
    
   #if EL_PRO
    sub.addItem (OpenLastUsedSession, "Open Last Saved Session", 
        true, openLastUsedSession());
    sub.addItem (AskToSaveSessions, "Ask To Save Session", 
        true, askToSaveSession());
   #else
    sub.addItem (OpenLastUsedSession, "Open Last Saved Graph", 
        true, openLastUsedSession());
    sub.addItem (AskToSaveSessions, "Ask To Save Graph", 
        true, askToSaveSession());
   #endif
   
    menu.addSubMenu ("General", sub);

    menu.addSeparator();

    int index = 0; sub.clear();
    for (const auto& device : MidiInput::getDevices())
        sub.addItem (MidiInputDevice + index++, device, true, 
            midi.isMidiInputEnabled (device));
    menu.addSubMenu ("MIDI Input Devices", sub);

    index = 0; sub.clear();
    for (const auto& device : MidiOutput::getDevices())
        sub.addItem (MidiOutputDevice + index++, device, true, 
            device == midi.getDefaultMidiOutputName());
    menu.addSubMenu ("MIDI Ouptut Device", sub);

    if (auto* type = devices.getCurrentDeviceTypeObject())
    {
        AudioDeviceManager::AudioDeviceSetup setup;
        devices.getAudioDeviceSetup (setup);
        menu.addSeparator();
        if (type->getTypeName() != "ASIO")
        {
            index = 0; sub.clear();
            for (const auto& device : type->getDeviceNames(true))
                sub.addItem(AudioInputDevice + index++, device, true,
                    device == setup.inputDeviceName);
            menu.addSubMenu("Audio Input Device", sub);
        }
        index = 0; sub.clear();
        for (const auto& device : type->getDeviceNames (false))
            sub.addItem (AudioOutputDevice + index++, device, true,
                device == setup.outputDeviceName);
        const auto menuName = type->getTypeName() == "ASIO"
            ? "Audio Device" : "Audio Output Device";
        menu.addSubMenu (menuName, sub);
    }

    if (auto* device = devices.getCurrentAudioDevice())
    {
        menu.addSeparator();
        index = 0; sub.clear();
        for (const auto rate : device->getAvailableSampleRates())
            sub.addItem (SampleRate + index++, String(int (rate)), true, 
                rate == device->getCurrentSampleRate());
        menu.addSubMenu ("Sample Rate", sub);

        index = 0; sub.clear();
        for (const auto bufSize : device->getAvailableBufferSizes())
            sub.addItem (BufferSize + index++, String(bufSize), true, 
                bufSize == device->getCurrentBufferSizeSamples());
        menu.addSubMenu ("Buffer Size", sub);
    }
}

bool Settings::performMenuResult (Globals& world, const int result)
{
    auto& devices (world.getDeviceManager());
    auto& midi (world.getMidiEngine());
    bool handled = true;

    switch (result)
    {
        case CheckForUpdatesOnStart: setCheckForUpdates (! checkForUpdates()); break;
        case ScanFormPluginsOnStart: setScanForPluginsOnStartup (! scanForPluginsOnStartup()); break;
        case AutomaticallyShowPluginWindows: setShowPluginWindowsWhenAdded (! showPluginWindowsWhenAdded()); break;
        case PluginWindowsOnTop: setPluginWindowsOnTop (! pluginWindowsOnTop()); break;
        case HidePluginWindowsWhenFocusLost: setHidePluginWindowsWhenFocusLost (! hidePluginWindowsWhenFocusLost()); break;
        case OpenLastUsedSession: setOpenLastUsedSession (! openLastUsedSession()); break;
        case AskToSaveSessions: setAskToSaveSession (! askToSaveSession()); break;
        default: handled = false; break;
    }

    if (handled)
    {
        saveIfNeeded();
        return true;
    }

    handled = true;
    if (settingResultIsFor (result, MidiInputDevice))
    {
        // MIDI input device
        const auto device = MidiInput::getDevices()[result - MidiInputDevice];
        if (device.isNotEmpty())
            midi.setMidiInputEnabled (device, ! midi.isMidiInputEnabled (device));
    }
    else if (settingResultIsFor (result, MidiOutputDevice))
    {
        // MIDI Output device
        const auto device = MidiOutput::getDevices()[result - MidiOutputDevice];
        if (device.isNotEmpty() && device == midi.getDefaultMidiOutputName())
            midi.setDefaultMidiOutput ({});
        else if (device.isNotEmpty())
            midi.setDefaultMidiOutput (device);
    }
    else if (settingResultIsFor (result, AudioInputDevice))
    {
        // Audio input device
        if (auto* type = devices.getCurrentDeviceTypeObject())
        {
            AudioDeviceManager::AudioDeviceSetup setup;
            devices.getAudioDeviceSetup (setup);
            const auto device = type->getDeviceNames(true)[result - AudioInputDevice];
            if (device.isNotEmpty() && device != setup.inputDeviceName)
            {
                setup.inputDeviceName = device;
                if (type->getTypeName() == "ASIO")
                    setup.outputDeviceName = device;
                devices.setAudioDeviceSetup (setup, true);
            }
        }
    }
    else if (settingResultIsFor (result, AudioOutputDevice))
    {
        // Audio output device
        if (auto* type = devices.getCurrentDeviceTypeObject())
        {
            AudioDeviceManager::AudioDeviceSetup setup;
            devices.getAudioDeviceSetup (setup);
            const auto device = type->getDeviceNames(false)[result - AudioOutputDevice];
            if (device.isNotEmpty() && device != setup.outputDeviceName)
            {
                if (type->getTypeName() == "ASIO")
                    setup.inputDeviceName = device;
                setup.outputDeviceName = device;
                devices.setAudioDeviceSetup (setup, true);
            }
        }
    }
    else if (settingResultIsFor (result, SampleRate))
    {
        // sample rate
        if (auto* device = devices.getCurrentAudioDevice())
        {
            const auto rate = device->getAvailableSampleRates()[result - SampleRate];
            if (rate > 0 && rate != device->getCurrentSampleRate())
            {
                AudioDeviceManager::AudioDeviceSetup setup;
                devices.getAudioDeviceSetup (setup);
                setup.sampleRate = rate;
                devices.setAudioDeviceSetup (setup, false);
            }
        }
    }
    else if (settingResultIsFor (result, BufferSize))
    {
        // buffer size
        if (auto* device = devices.getCurrentAudioDevice())
        {
            const auto bufSize = device->getAvailableBufferSizes()[result - BufferSize];
            if (bufSize > 0 && bufSize != device->getCurrentBufferSizeSamples())
            {
                AudioDeviceManager::AudioDeviceSetup setup;
                devices.getAudioDeviceSetup (setup);
                setup.bufferSize = bufSize;
                devices.setAudioDeviceSetup (setup, false);
            }
        }
    }
    else
    {
        handled = false;
    }

    if (handled)
        saveIfNeeded();

    return handled;
}

}

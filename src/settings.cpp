// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/context.hpp>
#include <element/devices.hpp>
#include <element/settings.hpp>

#include "appinfo.hpp"
#include "engine/midiengine.hpp"
#include "engine/midipanic.hpp"

namespace element {

const char* Settings::checkForUpdatesKey = "checkForUpdates";
const char* Settings::pluginFormatsKey = "pluginFormatsKey";
const char* Settings::pluginWindowOnTopDefault = "pluginWindowOnTopDefault";
const char* Settings::scanForPluginsOnStartKey = "scanForPluginsOnStart";
const char* Settings::showPluginWindowsKey = "showPluginWindows";
const char* Settings::openLastUsedSessionKey = "openLastUsedSession";
const char* Settings::askToSaveSessionKey = "askToSaveSession";
const char* Settings::defaultNewSessionFile = "defaultNewSessionFile";
const char* Settings::generateMidiClockKey = "generateMidiClockKey";
const char* Settings::sendMidiClockToInputKey = "sendMidiClockToInputKey";
const char* Settings::hidePluginWindowsWhenFocusLostKey = "hidePluginWindowsWhenFocusLost";
const char* Settings::lastGraphKey = "lastGraph";
const char* Settings::lastSessionKey = "lastSession";
const char* Settings::legacyInterfaceKey = "legacyInterface";
const char* Settings::midiEngineKey = "midiEngine";
const char* Settings::oscHostPortKey = "oscHostPortKey";
const char* Settings::oscHostEnabledKey = "oscHostEnabledKey";
const char* Settings::systrayKey = "systrayKey";
const char* Settings::startHiddenKey = "startHidden";
const char* Settings::midiOutLatencyKey = "midiOutLatency";
const char* Settings::desktopScaleKey = "desktopScale";
const char* Settings::mainContentTypeKey = "mainContentType";
const char* Settings::pluginListHeaderKey = "pluginListHeader";
const char* Settings::devicesKey = "devices";
const char* Settings::keymappingsKey = "keymappings";
const char* Settings::clockSourceKey = "clockSource";
const char* Settings::updateChannelKey = "updateChannel";
const char* Settings::updateKeyTypeKey = "updateKeyType";
const char* Settings::updateKeyKey = "updateKey";
const char* Settings::updateKeyUserKey = "updateKeyUserKey";
const char* Settings::authPreviewUpdatesKey = "authPreviewUpdates";
const char* Settings::authAppcastUrlKey = "authAppcastUrl";
const char* Settings::transportStartStopContinue = "transportStartStopContinueKey";

//=============================================================================
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

static bool settingResultIsFor (const int result, const int optionMenuId)
{
    return (result >= optionMenuId && result < optionMenuId + 1000000);
}

#if JUCE_32BIT
#if JUCE_MAC
const char* Settings::lastPluginScanPathPrefix = "pluginScanPath32_";
const char* Settings::pluginListKey = "pluginList32";

#else
const char* Settings::lastPluginScanPathPrefix = "pluginScanPath_"; // TODO: migrate this
const char* Settings::pluginListKey = "plugin-list"; // TODO: migrate this
#endif

#else // 64bit keys
#if JUCE_MAC
const char* Settings::lastPluginScanPathPrefix = "pluginScanPath_"; // TODO: migrate this
const char* Settings::pluginListKey = "plugin-list"; // TODO: migrate this

#else
const char* Settings::lastPluginScanPathPrefix = "pluginScanPath64_";
const char* Settings::pluginListKey = "pluginList64";
#endif
#endif

Settings::Settings()
{
    PropertiesFile::Options opts;
    opts.applicationName = EL_APP_NAME;
    opts.filenameSuffix = "conf";
    opts.osxLibrarySubFolder = "Application Support";
    opts.storageFormat = PropertiesFile::storeAsXML;

#if JUCE_DEBUG
    opts.applicationName << "_Debug";
#endif

#if JUCE_LINUX
    opts.folderName = String (".config/@0@").replace ("@0@", EL_APP_DATA_SUBDIR);
#else
    opts.folderName = EL_APP_DATA_SUBDIR;
#endif

    setStorageParameters (opts);
}

Settings::~Settings() {}

//=============================================================================
PropertiesFile* Settings::getProps() const
{
    return (const_cast<Settings*> (this))->getUserSettings();
}

bool Settings::getBool (std::string_view key, bool fallback) const noexcept
{
    auto p = getProps();
    return p != nullptr ? p->getBoolValue (key.data(), fallback) : fallback;
}

int Settings::getInt (std::string_view key, int fallback) const noexcept
{
    auto p = getProps();
    return p != nullptr ? p->getIntValue (key.data(), fallback) : fallback;
}

double Settings::getDouble (std::string_view key, double fallback) const noexcept
{
    auto p = getProps();
    return p != nullptr ? p->getDoubleValue (key.data(), fallback) : fallback;
}

String Settings::getString (std::string_view key, const String& fallback) const
{
    auto p = getProps();
    return p != nullptr ? p->getValue (key.data(), fallback) : fallback;
}

void Settings::set (std::string_view key, const var& value)
{
    if (auto p = getProps())
        p->setValue (key.data(), value);
}

//=============================================================================
bool Settings::checkForUpdates() const { return getBool (checkForUpdatesKey, true); }
void Settings::setCheckForUpdates (const bool shouldCheck) { set (checkForUpdatesKey, shouldCheck); }

std::unique_ptr<XmlElement> Settings::getLastGraph() const
{
    if (auto* p = getProps())
        return p->getXmlValue (lastGraphKey);
    return nullptr;
}

void Settings::setLastGraph (const ValueTree& data)
{
    jassert (data.hasType (types::Node));
    if (! data.hasType (types::Node))
        return;
    if (auto* p = getProps())
        if (auto xml = data.createXml())
            p->setValue (lastGraphKey, xml.get());
}

bool Settings::scanForPluginsOnStartup() const { return getBool (scanForPluginsOnStartKey, false); }
void Settings::setScanForPluginsOnStartup (const bool shouldScan) { set (scanForPluginsOnStartKey, shouldScan); }

bool Settings::showPluginWindowsWhenAdded() const { return getBool (showPluginWindowsKey, false); }
void Settings::setShowPluginWindowsWhenAdded (const bool shouldShow) { set (showPluginWindowsKey, shouldShow); }

bool Settings::openLastUsedSession() const { return getBool (openLastUsedSessionKey, true); }
void Settings::setOpenLastUsedSession (const bool shouldOpen) { set (openLastUsedSessionKey, shouldOpen); }

bool Settings::generateMidiClock() const { return getBool (generateMidiClockKey, false); }
void Settings::setGenerateMidiClock (const bool generate) { set (generateMidiClockKey, generate); }

bool Settings::pluginWindowsOnTop() const { return getBool (pluginWindowOnTopDefault, true); }
void Settings::setPluginWindowsOnTop (const bool onTop) { set (pluginWindowOnTopDefault, onTop); }

bool Settings::askToSaveSession() const { return getBool (askToSaveSessionKey, true); }
void Settings::setAskToSaveSession (const bool value) { set (askToSaveSessionKey, value); }

bool Settings::sendMidiClockToInput() const { return getBool (sendMidiClockToInputKey, false); }
void Settings::setSendMidiClockToInput (const bool value) { set (sendMidiClockToInputKey, value); }

const File Settings::getDefaultNewSessionFile() const
{
    const auto value = getString (defaultNewSessionFile);
    if (value.isNotEmpty() && File::isAbsolutePath (value))
        return File (value);
    return File();
}

void Settings::setDefaultNewSessionFile (const File& file)
{
    set (defaultNewSessionFile, file.existsAsFile() ? file.getFullPathName() : String());
}

bool Settings::hidePluginWindowsWhenFocusLost() const
{
#if JUCE_LINUX
    const bool fallback = false;
#else
    const bool fallback = true;
#endif
    return getBool (hidePluginWindowsWhenFocusLostKey, fallback);
}

void Settings::setHidePluginWindowsWhenFocusLost (const bool hideThem) { set (hidePluginWindowsWhenFocusLostKey, hideThem); }

bool Settings::useLegacyInterface() const { return getBool (legacyInterfaceKey, false); }
void Settings::setUseLegacyInterface (const bool useLegacy) { set (legacyInterfaceKey, useLegacy); }

//=============================================================================
bool Settings::isOscHostEnabled() const { return getBool (oscHostEnabledKey, false); }
void Settings::setOscHostEnabled (bool enabled) { set (oscHostEnabledKey, enabled); }

int Settings::getOscHostPort() const { return getInt (oscHostPortKey, 9000); }
void Settings::setOscHostPort (int port) { set (oscHostPortKey, port); }

//=============================================================================
bool Settings::isSystrayEnabled() const
{
#if JUCE_LINUX
    const bool fallback = false;
#else
    const bool fallback = true;
#endif
    return getBool (systrayKey, fallback);
}

void Settings::setSystrayEnabled (bool enabled) { set (systrayKey, enabled); }

bool Settings::isStartHiddenEnabled() const { return getBool (startHiddenKey, false); }
void Settings::setStartHiddenEnabled (bool enabled) { set (startHiddenKey, enabled); }

//=============================================================================
double Settings::getMidiOutLatency() const { return getDouble (midiOutLatencyKey, 0.0); }
void Settings::setMidiOutLatency (double latencyMs) { set (midiOutLatencyKey, latencyMs); }

double Settings::getDesktopScale() const { return getDouble (desktopScaleKey, 1.0); }
void Settings::setDesktopScale (double scale) { set (desktopScaleKey, jlimit (0.1, 8.0, scale)); }

//=============================================================================
String Settings::getMainContentType() const
{
    auto value = getString (mainContentTypeKey, "standard");
    if (value != "standard" && value != "menubarOnly")
        return "standard";
    return value;
}

void Settings::setMainContentType (const String& tp)
{
    if (tp != "standard" && tp != "menubarOnly")
    {
        jassertfalse;
        return;
    }
    set (mainContentTypeKey, tp);
}

String Settings::getClockSource() const { return getString (clockSourceKey, "internal"); }

void Settings::setClockSource (const String& src)
{
    if (src != "internal" && src != "midiClock")
    {
        jassertfalse;
        return;
    }
    set (clockSourceKey, src);
}

//=============================================================================
String Settings::getUpdateKeyType() const { return getString (updateKeyTypeKey, "element-v1"); }

void Settings::setUpdateKeyType (const String& slug)
{
    if (slug != "patreon" && slug != "element-v1" && slug != "member")
    {
        jassertfalse;
        return;
    }
    set (updateKeyTypeKey, slug);
}

String Settings::getUpdateKeyUser() const { return getString (updateKeyUserKey); }
void Settings::setUpdateKeyUser (const String& user) { set (updateKeyUserKey, user.trim()); }

String Settings::getUpdateKey() const { return getString (updateKeyKey); }

void Settings::setUpdateKey (const String& slug)
{
    set (updateKeyKey, slug.trim());
    sendChangeMessage();
}

String Settings::getUpdateChannel() const { return getString (updateChannelKey); }
void Settings::setUpdateChannel (const String& channel) { set (updateChannelKey, channel.trim()); }

bool Settings::getAuthPreviewUpdates() const { return getBool (authPreviewUpdatesKey, false); }

void Settings::setAuthPreviewUpdates (bool enabled)
{
    set (authPreviewUpdatesKey, enabled);
    sendChangeMessage();
}

String Settings::getAuthAppcastUrl() const { return getString (authAppcastUrlKey); }

void Settings::setAuthAppcastUrl (const String& url)
{
    set (authAppcastUrlKey, url);
    sendChangeMessage();
}

//=============================================================================
void Settings::setMidiPanicParams (MidiPanicParams params)
{
    set ("midiPanicCCEnabled", params.enabled);
    set ("midiPanicCCNumber", params.ccNumber);
    set ("midiPanicChannel", params.channel);
}

MidiPanicParams Settings::getMidiPanicParams() const
{
    MidiPanicParams params;
    params.enabled = getBool ("midiPanicCCEnabled", false);
    params.ccNumber = getInt ("midiPanicCCNumber", -1);
    params.channel = getInt ("midiPanicChannel", 1);
    return params;
}

bool Settings::transportRespondToStartStopContinue() const { return getBool (transportStartStopContinue, false); }
void Settings::setTransportRespondToStartStopContinue (bool shouldRespond) { set (transportStartStopContinue, shouldRespond); }

//=============================================================================
void Settings::addItemsToMenu (Context& world, PopupMenu& menu)
{
    auto& devices (world.devices());
    auto& midi (world.midi());
    PopupMenu sub;

    sub.addItem (CheckForUpdatesOnStart, "Check Updates at Startup", true, checkForUpdates());

    sub.addSeparator(); // plugins items

    sub.addItem (ScanFormPluginsOnStart, "Scan Plugins at Startup", true, scanForPluginsOnStartup());
    sub.addItem (AutomaticallyShowPluginWindows, "Automatically Show Plugin Windows", true, showPluginWindowsWhenAdded());
    sub.addItem (PluginWindowsOnTop, "Plugins On Top By Default", true, pluginWindowsOnTop());
    sub.addItem (HidePluginWindowsWhenFocusLost, "Hide Plugin Windows When App Inactive", true, hidePluginWindowsWhenFocusLost());

    sub.addSeparator(); // session items

#if ! ELEMENT_SE
    const String sessTxt = "Session";
#else
    const String sessTxt = "Graph";
#endif

    sub.addItem (OpenLastUsedSession,
                 String ("Open Last Saved ") + sessTxt,
                 true,
                 openLastUsedSession());
    sub.addItem (AskToSaveSessions,
                 String ("Ask To Save ") + sessTxt,
                 true,
                 askToSaveSession());

    menu.addSubMenu ("General", sub);

    menu.addSeparator();

    int index = 0;
    sub.clear();
    for (const auto& device : MidiInput::getAvailableDevices())
        sub.addItem (MidiInputDevice + index++, device.name, true, midi.isMidiInputEnabled (device));
    menu.addSubMenu ("MIDI Input Devices", sub);

    index = 0;
    sub.clear();
    for (const auto& device : MidiOutput::getAvailableDevices())
        sub.addItem (MidiOutputDevice + index++, device.name, true, device.identifier == midi.getDefaultMidiOutputID());
    menu.addSubMenu ("MIDI Output Device", sub);

    if (auto* type = devices.getCurrentDeviceTypeObject())
    {
        AudioDeviceManager::AudioDeviceSetup setup;
        devices.getAudioDeviceSetup (setup);
        menu.addSeparator();
        if (type->getTypeName() != "ASIO")
        {
            index = 0;
            sub.clear();
            for (const auto& device : type->getDeviceNames (true))
                sub.addItem (AudioInputDevice + index++, device, true, device == setup.inputDeviceName);
            menu.addSubMenu ("Audio Input Device", sub);
        }
        index = 0;
        sub.clear();
        for (const auto& device : type->getDeviceNames (false))
            sub.addItem (AudioOutputDevice + index++, device, true, device == setup.outputDeviceName);
        const auto menuName = type->getTypeName() == "ASIO"
                                  ? "Audio Device"
                                  : "Audio Output Device";
        menu.addSubMenu (menuName, sub);
    }

    if (auto* device = devices.getCurrentAudioDevice())
    {
        menu.addSeparator();
        index = 0;
        sub.clear();
        for (const auto rate : device->getAvailableSampleRates())
            sub.addItem (SampleRate + index++, String (int (rate)), true, rate == device->getCurrentSampleRate());
        menu.addSubMenu ("Sample Rate", sub);

        index = 0;
        sub.clear();
        for (const auto bufSize : device->getAvailableBufferSizes())
            sub.addItem (BufferSize + index++, String (bufSize), true, bufSize == device->getCurrentBufferSizeSamples());
        menu.addSubMenu ("Buffer Size", sub);
    }
}

bool Settings::performMenuResult (Context& world, const int result)
{
    auto& devices (world.devices());
    auto& midi (world.midi());
    bool handled = true;

    switch (result)
    {
        case CheckForUpdatesOnStart:
            setCheckForUpdates (! checkForUpdates());
            break;
        case ScanFormPluginsOnStart:
            setScanForPluginsOnStartup (! scanForPluginsOnStartup());
            break;
        case AutomaticallyShowPluginWindows:
            setShowPluginWindowsWhenAdded (! showPluginWindowsWhenAdded());
            break;
        case PluginWindowsOnTop:
            setPluginWindowsOnTop (! pluginWindowsOnTop());
            break;
        case HidePluginWindowsWhenFocusLost:
            setHidePluginWindowsWhenFocusLost (! hidePluginWindowsWhenFocusLost());
            break;
        case OpenLastUsedSession:
            setOpenLastUsedSession (! openLastUsedSession());
            break;
        case AskToSaveSessions:
            setAskToSaveSession (! askToSaveSession());
            break;
        default:
            handled = false;
            break;
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
        const auto device = MidiInput::getAvailableDevices()[result - MidiInputDevice];
        if (device.identifier.isNotEmpty())
            midi.setMidiInputEnabled (device, ! midi.isMidiInputEnabled (device));
    }
    else if (settingResultIsFor (result, MidiOutputDevice))
    {
        // MIDI Output device
        const auto device = MidiOutput::getAvailableDevices()[result - MidiOutputDevice];
        if (device.identifier.isNotEmpty() && device.identifier == midi.getDefaultMidiOutputID())
            midi.setDefaultMidiOutput ({});
        else if (device.identifier.isNotEmpty())
            midi.setDefaultMidiOutput (device);
    }
    else if (settingResultIsFor (result, AudioInputDevice))
    {
        // Audio input device
        if (auto* type = devices.getCurrentDeviceTypeObject())
        {
            AudioDeviceManager::AudioDeviceSetup setup;
            devices.getAudioDeviceSetup (setup);
            const auto device = type->getDeviceNames (true)[result - AudioInputDevice];
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
            const auto device = type->getDeviceNames (false)[result - AudioOutputDevice];
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
                devices.setAudioDeviceSetup (setup, true);
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
                devices.setAudioDeviceSetup (setup, true);
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

} // namespace element

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/audio_devices.hpp>
#include <element/audioengine.hpp>
#include <element/signals.hpp>

namespace element {

class DeviceManager : public juce::AudioDeviceManager {
public:
    typedef juce::AudioDeviceManager::AudioDeviceSetup AudioSettings;
    static const int maxAudioChannels;

    DeviceManager();
    ~DeviceManager();

    void getAudioDrivers (juce::StringArray& drivers);
    void selectAudioDriver (const juce::String& name);
    void attach (AudioEnginePtr engine);

    /** Fires on the message thread whenever an audio device type reports that
        its device list changed, e.g. an interface was plugged in or removed —
        or, on Windows, when audio-class hardware arrived or was removed
        (ASIO cannot report this itself). */
    Signal<void()> sigDeviceListChanged;

    /** Checks whether a device is physically present in a device type's list.

        Unlike querying getAvailableDeviceTypes() directly, this always checks
        the real hardware list, ignoring the hold that keeps a disconnected
        device visible while it is still open (see WatchedDeviceType).

        @param typeName   The audio device type name (e.g. "CoreAudio")
        @param deviceName The device name to look for
        @param rescan     If true, rescan the type for devices before checking
        @return true if the device is present in the type's hardware list
    */
    bool isDevicePresent (const juce::String& typeName, const juce::String& deviceName, bool rescan);

#if KV_JACK_AUDIO
    kv::JackClient& getJackClient();
#endif

    void createAudioDeviceTypes (juce::OwnedArray<juce::AudioIODeviceType>& list) override;

private:
    friend class World;
    class Private;
    std::unique_ptr<Private> impl;
};

} // namespace element

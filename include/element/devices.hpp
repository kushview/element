// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/audio_devices.hpp>
#include <element/audioengine.hpp>

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

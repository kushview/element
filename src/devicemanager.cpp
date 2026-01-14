// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/devices.hpp>
#include "engine/jack.hpp"

namespace element {

using namespace juce;

const int DeviceManager::maxAudioChannels = 128;

class DeviceManager::Private
{
public:
    Private (DeviceManager& o) : owner (o) {}
    ~Private() {}

    DeviceManager& owner;
    AudioEnginePtr engine;
#if ELEMENT_USE_JACK
    JackClient jack { "Element", 2, "main_in_", 2, "main_out_" };
#endif

    juce::ReferenceCountedArray<DeviceManager::LevelMeter> levelsIn, levelsOut;
};

DeviceManager::DeviceManager()
{
    impl = std::make_unique<Private> (*this);
}

DeviceManager::~DeviceManager()
{
    closeAudioDevice();
    attach (nullptr);
}

void DeviceManager::attach (AudioEnginePtr engine)
{
    if (impl->engine == engine)
        return;

    auto old = impl->engine;

    if (old != nullptr)
    {
        removeAudioCallback (&old->getAudioIODeviceCallback());
    }

    if (engine)
    {
        addAudioCallback (&engine->getAudioIODeviceCallback());
    }
    else
    {
        closeAudioDevice();
    }

    impl->engine = engine;
}

static void addIfNotNull (OwnedArray<AudioIODeviceType>& list, AudioIODeviceType* const device)
{
    if (device != nullptr)
        list.add (device);
}

void DeviceManager::createAudioDeviceTypes (OwnedArray<AudioIODeviceType>& list)
{
#if JUCE_ALSA
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_ALSA());
#endif
#if ELEMENT_USE_JACK
    addIfNotNull (list, Jack::createAudioIODeviceType (impl->jack));
#endif

    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_ASIO());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_WASAPI (WASAPIDeviceMode::exclusive));
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_WASAPI (WASAPIDeviceMode::sharedLowLatency));
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_DirectSound());

    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_CoreAudio());

    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_iOSAudio());

    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_OpenSLES());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_Android());
}

void DeviceManager::getAudioDrivers (StringArray& drivers)
{
    const OwnedArray<AudioIODeviceType>& types (getAvailableDeviceTypes());
    for (int i = 0; i < types.size(); ++i)
        drivers.add (types.getUnchecked (i)->getTypeName());
}

void DeviceManager::selectAudioDriver (const String& name)
{
    setCurrentAudioDeviceType (name, true);
}

#if KV_JACK_AUDIO
kv::JackClient& DeviceManager::getJackClient()
{
    return impl->jack;
}
#endif

} // namespace element

/*
    This file is part of Element
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

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

#include <element/devicemanager.hpp>

namespace element {

using namespace juce;

const int DeviceManager::maxAudioChannels = 128;

class DeviceManager::Private
{
public:
    Private() {}
    ~Private() {}

    EnginePtr activeEngine;
#if KV_JACK_AUDIO
    kv::JackClient jackClient { "Element", 2, "main_in_", 2, "main_out_" };
#endif

    juce::ReferenceCountedArray<DeviceManager::LevelMeter> levelsIn, levelsOut;
};

DeviceManager::DeviceManager()
{
    impl = std::make_unique<Private>();
}

DeviceManager::~DeviceManager()
{
    closeAudioDevice();
    attach (nullptr);
}

DeviceManager::LevelMeter::Ptr getLevelMeter (int channel, bool input)
{
    juce::ignoreUnused (channel, input);
}

void DeviceManager::attach (EnginePtr engine)
{
    if (impl->activeEngine == engine)
        return;

    EnginePtr old = impl->activeEngine;

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

    impl->activeEngine = engine;
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
#if KV_JACK_AUDIO
    addIfNotNull (list, Jack::createAudioIODeviceType (&impl->jackClient));
#endif

    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_ASIO());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_WASAPI (true));
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_WASAPI (false));
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
    return impl->jackClient;
}
#endif

} // namespace element

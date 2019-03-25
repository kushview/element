/*
    DeviceManager.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#include "session/DeviceManager.h"

namespace Element {

const int DeviceManager::maxAudioChannels   = 128;

class DeviceManager::Private
{
public:
    Private() { }
    ~Private() { }

    EnginePtr activeEngine;
};

DeviceManager::DeviceManager()
{
    impl = new Private();
}

DeviceManager::~DeviceManager()
{
    closeAudioDevice();
    attach (nullptr);
}

void DeviceManager::attach (EnginePtr engine)
{
    if (impl->activeEngine == engine)
        return;

    EnginePtr old = impl->activeEngine;

    if (old != nullptr)
    {
        removeAudioCallback (&old->getAudioIODeviceCallback());
        removeMidiInputCallback (String(), &old->getMidiInputCallback());
    }

    if (engine)
    {
        addAudioCallback (&engine->getAudioIODeviceCallback());
        addMidiInputCallback (String(), &engine->getMidiInputCallback());
    }
    else
    {
        closeAudioDevice();
    }

    impl->activeEngine = engine;
}

static void addIfNotNull (OwnedArray <AudioIODeviceType>& list, AudioIODeviceType* const device)
{
    if (device != nullptr)
        list.add (device);
}

void DeviceManager::createAudioDeviceTypes (OwnedArray <AudioIODeviceType>& list)
{
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_WASAPI (true));
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_WASAPI (false));
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_DirectSound());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_ASIO());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_CoreAudio());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_iOSAudio());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_ALSA());
   #if ELEMENT_USE_JACK
    addIfNotNull (list, Jack::createAudioIODeviceType (nullptr));
   #endif
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_OpenSLES());
    addIfNotNull (list, AudioIODeviceType::createAudioIODeviceType_Android());
}

void DeviceManager::getAudioDrivers (StringArray& drivers)
{
	const OwnedArray<AudioIODeviceType>& types (getAvailableDeviceTypes());
	for (int i = 0; i < types.size(); ++i)
		drivers.add (types.getUnchecked(i)->getTypeName());
}

void DeviceManager::selectAudioDriver (const String& name)
{
    setCurrentAudioDeviceType (name, true);
}

}

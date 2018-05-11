/*
    DeviceManager.h - This file is part of Element
    Copyright (C) 2014-2018  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "engine/Engine.h"

namespace Element {

class DeviceManager :  public AudioDeviceManager
{
public:
    typedef AudioDeviceManager::AudioDeviceSetup AudioSettings;
    static const int maxAudioChannels;
    
    DeviceManager();
    ~DeviceManager();

    void getAudioDrivers (StringArray& drivers);
    void selectAudioDriver (const String& name);
    void attach (EnginePtr engine);

    virtual void createAudioDeviceTypes (OwnedArray <AudioIODeviceType>& list) override;

private:
    friend class World;
    class Private;
    Scoped<Private> impl;
};

}

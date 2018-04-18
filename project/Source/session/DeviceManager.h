/*
    DeviceManager.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#ifndef EL_DEVICE_MANAGER_H
#define EL_DEVICE_MANAGER_H

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

    virtual void createAudioDeviceTypes (OwnedArray <AudioIODeviceType>& list) override;

    void getAudioDrivers (StringArray& drivers);
    void selectAudioDriver (const String& name);
    void attach (EnginePtr engine);
private:

    friend class World;

    class Private;
    Scoped<Private> impl;
};

}

#endif // EL_DEVICE_MANAGER_H

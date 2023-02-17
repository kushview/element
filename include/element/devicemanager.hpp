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

#pragma once

#include <element/juce/audio_devices.hpp>
#include <element/engine.hpp>

namespace element {

class DeviceManager : public juce::AudioDeviceManager {
public:
    typedef juce::AudioDeviceManager::AudioDeviceSetup AudioSettings;
    static const int maxAudioChannels;

    DeviceManager();
    ~DeviceManager();

    void getAudioDrivers (juce::StringArray& drivers);
    void selectAudioDriver (const juce::String& name);
    void attach (EnginePtr engine);

#if KV_JACK_AUDIO
    kv::JackClient& getJackClient();
#endif

    virtual void createAudioDeviceTypes (juce::OwnedArray<juce::AudioIODeviceType>& list) override;

private:
    friend class World;
    class Private;
    std::unique_ptr<Private> impl;
};

} // namespace element

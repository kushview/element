// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/audio_devices.hpp>

namespace element {

class Engine : public juce::ReferenceCountedObject {
public:
    virtual ~Engine() {}
    virtual juce::AudioIODeviceCallback& getAudioIODeviceCallback() = 0;
    virtual juce::MidiInputCallback& getMidiInputCallback() = 0;
};

typedef juce::ReferenceCountedObjectPtr<Engine> EnginePtr;

} // namespace element

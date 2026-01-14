// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/gui_basics.hpp>
#include <element/juce/audio_devices.hpp>

namespace element {

inline static void addMidiDevicesToMenu (juce::PopupMenu& menu, const bool isInput, const int offset = 80000)
{
    jassert (offset > 0);
    const auto devices = isInput ? juce::MidiInput::getAvailableDevices()
                                 : juce::MidiOutput::getAvailableDevices();
    for (int i = 0; i < devices.size(); ++i)
        menu.addItem (i + offset, devices[i].name, true, false);
}

inline static juce::MidiDeviceInfo getMidiDeviceForMenuResult (const int result, const bool isInput, const int offset = 80000)
{
    const int index = result - offset;
    const auto devices = isInput ? juce::MidiInput::getAvailableDevices()
                                 : juce::MidiOutput::getAvailableDevices();
    return juce::isPositiveAndBelow (index, devices.size()) ? devices.getUnchecked (index)
                                                            : juce::MidiDeviceInfo();
}

} // namespace element
/*
    This file is part of Element
    Copyright (C) 2023  Kushview, LLC.  All rights reserved.

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
    jassert (offset > 0 && result >= offset);
    const int index = result - offset;
    const auto devices = isInput ? juce::MidiInput::getAvailableDevices()
                                 : juce::MidiOutput::getAvailableDevices();
    return juce::isPositiveAndBelow (index, devices.size()) ? devices.getUnchecked (index)
                                                            : juce::MidiDeviceInfo();
}

} // namespace element
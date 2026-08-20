// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/audio_devices.hpp>
#include <element/services.hpp>
#include <element/signals.hpp>

#include "services/devicemonitor.hpp"

namespace element {

class DeviceService : public Service
{
public:
    DeviceService();
    ~DeviceService();

    void activate() override;
    void deactivate() override;

    /** Fires (on the message thread) whenever the set of available MIDI input or
        output devices changes, e.g. a controller is plugged in or removed. */
    Signal<void()> sigMidiDevicesChanged;

    /** Fires (on the message thread) when the audio device connection status
        changes: opened, disconnected and waiting, or no device selected. */
    Signal<void (const AudioDeviceMonitor::Status&)> sigAudioDeviceStatus;

    /** Returns the current audio device connection status. */
    AudioDeviceMonitor::Status audioDeviceStatus() const;

    /** Call when the system wakes from sleep to re-evaluate the audio device. */
    void onResume();

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl;

    /** Watches the system MIDI device list and drives sigMidiDevicesChanged. */
    juce::MidiDeviceListConnection deviceListConnection;
};

} // namespace element

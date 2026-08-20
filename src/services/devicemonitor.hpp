// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include <element/juce/core.hpp>
#include <element/signals.hpp>

namespace element {

/** Tracks the audio device's health and owns disconnect/reconnect policy.

    A pure state machine with no direct AudioDeviceManager dependency: all
    interaction with the real device stack goes through the Backend interface
    so the logic can be unit tested with a fake.

    Policy: when the desired device disconnects, close it and wait — never
    auto-select a replacement. When the desired device reappears, restore it
    with the last saved setup. User-initiated setup changes are persisted
    immediately. How reconnection is attempted depends on the device type's
    ReconnectPolicy: by polling presence, by polling opens when presence is
    undetectable but cheap to try, or — where a failed open stalls the
    message thread (ASIO) — only on platform hardware events plus a slow
    safety-net poll.

    All methods must be called from the message thread.
*/
class AudioDeviceMonitor
{
public:
    enum class State
    {
        inactive, //!< No device desired (user selected none, or plugin mode)
        active, //!< Desired device is open and running
        waiting //!< Desired device unavailable; waiting for it to return
    };

    struct Status
    {
        State state { State::inactive };
        juce::String deviceName;
    };

    /** Snapshot of the currently open audio device, if any. */
    struct DeviceInfo
    {
        bool open { false };
        bool playing { false };
        juce::String typeName;
        juce::String inputName, outputName;
    };

    /** How the monitor tries to reconnect a lost device of a given type. */
    enum class ReconnectPolicy
    {
        pollPresence, //!< Presence is detectable by rescanning; poll it, open when found.
        pollBlind, //!< Presence undetectable but a failed open is cheap (ALSA).
        eventDriven //!< Presence undetectable and a failed open blocks the message
        //!< thread for seconds (ASIO); open only on hardware events
        //!< plus a slow safety-net poll.
    };

    /** Bridge to the real device manager, engine, and settings. */
    struct Backend
    {
        virtual ~Backend() = default;

        /** Returns info about the currently open device. */
        virtual DeviceInfo currentDevice() = 0;

        /** Returns true if the named device exists in the type's hardware
            list. Rescans the type first when rescan is true. */
        virtual bool isDevicePresent (const juce::String& typeName,
                                      const juce::String& deviceName,
                                      bool rescan) = 0;

        /** Returns the reconnect policy for a device type. */
        virtual ReconnectPolicy reconnectPolicy (const juce::String& typeName) = 0;

        /** Tries to open the device described by a DEVICESETUP element.
            @return an empty string on success, otherwise the error */
        virtual juce::String attemptOpenDesired (const juce::XmlElement& setupXml) = 0;

        /** Closes the current audio device. */
        virtual void closeDevice() = 0;

        /** Returns the device manager's current explicit settings, or
            nullptr if none exist. */
        virtual std::unique_ptr<juce::XmlElement> stateXml() = 0;

        /** Persists device settings so they survive an unclean shutdown. */
        virtual void persist (const juce::XmlElement& xml) = 0;

        /** Returns the engine's audio IO callback counter. */
        virtual uint64_t ioTicks() = 0;

        /** Returns and clears the last mid-stream device error, if any. */
        virtual juce::String lastDeviceError() = 0;
    };

    /** Ticks of frozen audio callbacks before checking device presence. */
    static constexpr int staleTicksBeforeConfirm = 4;
    /** Ticks of frozen audio callbacks before forcing a disconnect even if
        the (possibly stale) device list still claims the device exists. */
    static constexpr int staleTicksBeforeForce = 8;
    /** Timer ticks between reconnect attempts while waiting. */
    static constexpr int reconnectPollTicks = 3;
    /** Timer ticks between safety-net reconnect attempts for event-driven
        types, in case a hardware notification was missed. */
    static constexpr int safetyNetPollTicks = 30;
    /** Fast retries (at reconnectPollTicks cadence) after a hardware event
        whose restore attempt failed — drivers often need a moment after
        the OS announces arrival. */
    static constexpr int hardwareEventRetries = 2;

    explicit AudioDeviceMonitor (Backend& backendToUse);

    /** Adopts the saved device settings and evaluates the initial state.
        Never attempts to open a device. */
    void seed (std::unique_ptr<juce::XmlElement> savedXml);

    /** Call (deferred) when the device manager broadcasts any change. */
    void onChangeEvent();

    /** Call (deferred) when the platform reported audio hardware arrival or
        removal. Unlike onChangeEvent, this may attempt a restore even when
        presence cannot be verified (event-driven policy). */
    void onHardwareEvent();

    /** Call about once per second from a timer. */
    void onTimerTick();

    /** Call when the system wakes from sleep. */
    void onResume();

    Status status() const { return current; }

    /** Fires on the message thread whenever the status changes. */
    Signal<void (const Status&)> sigStatusChanged;

private:
    Backend& backend;
    Status current;
    std::unique_ptr<juce::XmlElement> desiredXml;
    juce::String desiredType, desiredInput, desiredOutput;
    bool restoring { false };
    uint64_t lastTicks { 0 };
    int staleCount { 0 };
    int pollCount { 0 };
    int fastRetries { 0 };
    int retryCooldown { 0 };

    bool hasDesired() const noexcept;
    juce::String desiredDisplayName() const;
    void parseDesired();
    bool desiredPresent (bool rescan);
    bool matchesDesired (const DeviceInfo& device) const;
    void updateDesiredFromBackend();
    void attemptRestore();
    void enterWaiting();
    void setState (State newState, const juce::String& deviceName);
};

} // namespace element

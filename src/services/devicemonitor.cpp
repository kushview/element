// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "services/devicemonitor.hpp"

namespace element {

AudioDeviceMonitor::AudioDeviceMonitor (Backend& backendToUse)
    : backend (backendToUse)
{
}

bool AudioDeviceMonitor::hasDesired() const noexcept
{
    return desiredInput.isNotEmpty() || desiredOutput.isNotEmpty();
}

juce::String AudioDeviceMonitor::desiredDisplayName() const
{
    return desiredOutput.isNotEmpty() ? desiredOutput : desiredInput;
}

void AudioDeviceMonitor::parseDesired()
{
    desiredType = desiredInput = desiredOutput = juce::String();
    if (desiredXml == nullptr)
        return;

    desiredType = desiredXml->getStringAttribute ("deviceType");

    // Attribute names match AudioDeviceManager's DEVICESETUP format,
    // including the legacy single-name variant.
    const auto legacyName = desiredXml->getStringAttribute ("audioDeviceName");
    if (legacyName.isNotEmpty())
    {
        desiredInput = desiredOutput = legacyName;
    }
    else
    {
        desiredInput = desiredXml->getStringAttribute ("audioInputDeviceName");
        desiredOutput = desiredXml->getStringAttribute ("audioOutputDeviceName");
    }
}

bool AudioDeviceMonitor::desiredPresent (bool rescan)
{
    if (! hasDesired())
        return false;

    if (desiredInput.isNotEmpty())
    {
        if (! backend.isDevicePresent (desiredType, desiredInput, rescan))
            return false;
        rescan = false;
    }

    if (desiredOutput.isNotEmpty() && desiredOutput != desiredInput)
        if (! backend.isDevicePresent (desiredType, desiredOutput, rescan))
            return false;

    return true;
}

bool AudioDeviceMonitor::matchesDesired (const DeviceInfo& device) const
{
    if (desiredType.isNotEmpty() && device.typeName != desiredType)
        return false;
    return device.inputName == desiredInput && device.outputName == desiredOutput;
}

void AudioDeviceMonitor::setState (State newState, const juce::String& deviceName)
{
    if (current.state == newState && current.deviceName == deviceName)
        return;
    current.state = newState;
    current.deviceName = deviceName;
    sigStatusChanged (current);
}

void AudioDeviceMonitor::seed (std::unique_ptr<juce::XmlElement> savedXml)
{
    desiredXml = std::move (savedXml);
    parseDesired();

    const auto device = backend.currentDevice();
    lastTicks = backend.ioTicks();
    staleCount = pollCount = 0;

    if (device.open)
        setState (State::active, device.outputName.isNotEmpty() ? device.outputName : device.inputName);
    else if (hasDesired())
        setState (State::waiting, desiredDisplayName());
    else
        setState (State::inactive, {});
}

void AudioDeviceMonitor::updateDesiredFromBackend()
{
    auto xml = backend.stateXml();
    if (xml == nullptr)
        return;

    if (desiredXml == nullptr || ! xml->isEquivalentTo (desiredXml.get(), true))
    {
        backend.persist (*xml);
        desiredXml = std::move (xml);
        parseDesired();
    }
}

void AudioDeviceMonitor::enterWaiting()
{
    staleCount = 0;
    pollCount = 0;
    if (backend.currentDevice().open)
        backend.closeDevice();
    setState (State::waiting, desiredDisplayName());
}

void AudioDeviceMonitor::attemptRestore()
{
    if (desiredXml == nullptr || ! hasDesired())
    {
        setState (State::inactive, {});
        return;
    }

    restoring = true;
    const auto error = backend.attemptOpenDesired (*desiredXml);
    restoring = false;

    if (error.isEmpty() && backend.currentDevice().open)
    {
        // The device may have reopened with adjusted parameters (e.g. an
        // unsupported sample rate); resync without touching saved settings.
        if (auto xml = backend.stateXml())
        {
            desiredXml = std::move (xml);
            parseDesired();
        }
        lastTicks = backend.ioTicks();
        staleCount = 0;
        setState (State::active, desiredDisplayName());
    }
    else
    {
        setState (State::waiting, desiredDisplayName());
    }
}

void AudioDeviceMonitor::onChangeEvent()
{
    if (restoring)
        return;

    const auto device = backend.currentDevice();

    switch (current.state)
    {
        case State::active: {
            // Also runs when the device just closed: if the user selected
            // "no device" the new state has empty names and we go inactive
            // rather than trying to reopen it.
            updateDesiredFromBackend();

            if (! hasDesired())
            {
                setState (State::inactive, {});
                break;
            }

            if (! desiredPresent (false))
                enterWaiting();
            else if (! device.open)
                enterWaiting(); // closed externally; the poll retries reopening
            else
                setState (State::active, desiredDisplayName());
            break;
        }

        case State::waiting: {
            if (device.open)
            {
                // A user-chosen setup updates the manager's explicit
                // settings; adopt it. A device opened behind our back does
                // not, and gets closed to keep the no-substitution policy.
                updateDesiredFromBackend();
                if (matchesDesired (device))
                {
                    lastTicks = backend.ioTicks();
                    staleCount = pollCount = 0;
                    setState (State::active, desiredDisplayName());
                }
                else
                {
                    backend.closeDevice();
                }
                break;
            }

            if (desiredPresent (false))
                attemptRestore();
            break;
        }

        case State::inactive: {
            if (device.open)
            {
                updateDesiredFromBackend();
                if (hasDesired())
                    setState (State::active, desiredDisplayName());
            }
            break;
        }
    }
}

void AudioDeviceMonitor::onTimerTick()
{
    switch (current.state)
    {
        case State::active: {
            const bool errored = backend.lastDeviceError().isNotEmpty();
            const auto device = backend.currentDevice();
            const auto ticks = backend.ioTicks();
            const bool frozen = device.open && device.playing && ticks == lastTicks;
            lastTicks = ticks;

            if (errored)
                staleCount = juce::jmax (staleCount + 1, staleTicksBeforeConfirm);
            else if (frozen)
                ++staleCount;
            else
                staleCount = 0;

            if (staleCount >= staleTicksBeforeForce)
            {
                // The stream is dead even though the device list still
                // claims the device exists (stale ALSA cache, dead ASIO
                // driver still registered).
                enterWaiting();
            }
            else if (staleCount >= staleTicksBeforeConfirm)
            {
                if (! desiredPresent (true))
                    enterWaiting();
            }
            break;
        }

        case State::waiting: {
            if (backend.currentDevice().open)
                break; // change event pending; onChangeEvent decides

            if (++pollCount < reconnectPollTicks)
                break;
            pollCount = 0;

            if (! backend.presenceDetectable (desiredType))
                attemptRestore(); // a failed open of a missing device fails fast
            else if (desiredPresent (true))
                attemptRestore();
            break;
        }

        case State::inactive:
            break;
    }
}

void AudioDeviceMonitor::onResume()
{
    staleCount = 0;
    lastTicks = backend.ioTicks();

    if (current.state == State::waiting && ! backend.currentDevice().open)
    {
        pollCount = 0;
        if (! backend.presenceDetectable (desiredType) || desiredPresent (true))
            attemptRestore();
    }
}

} // namespace element

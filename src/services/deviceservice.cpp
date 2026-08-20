// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/audioengine.hpp>
#include <element/context.hpp>
#include <element/devices.hpp>
#include <element/juce/events.hpp>
#include <element/settings.hpp>

#include "services/deviceservice.hpp"

namespace element {

namespace {

/** Bridges the monitor to the live device manager, engine, and settings. */
class ContextBackend : public AudioDeviceMonitor::Backend
{
public:
    explicit ContextBackend (Context& ctx) : context (ctx) {}

    AudioDeviceMonitor::DeviceInfo currentDevice() override
    {
        AudioDeviceMonitor::DeviceInfo info;
        auto& devices = context.devices();
        if (auto* const device = devices.getCurrentAudioDevice())
        {
            const auto setup = devices.getAudioDeviceSetup();
            info.open = true;
            info.playing = device->isPlaying();
            info.typeName = device->getTypeName();
            info.inputName = setup.inputDeviceName;
            info.outputName = setup.outputDeviceName;
        }
        return info;
    }

    bool isDevicePresent (const juce::String& typeName,
                          const juce::String& deviceName,
                          bool rescan) override
    {
        return context.devices().isDevicePresent (typeName, deviceName, rescan);
    }

    bool presenceDetectable (const juce::String& typeName) override
    {
        // ALSA's device list is scanned once and cached for the lifetime of
        // the type object, so absence can never be re-detected there.
        return typeName != "ALSA";
    }

    juce::String attemptOpenDesired (const juce::XmlElement& xml) override
    {
        auto& devices = context.devices();

        juce::AudioDeviceManager::AudioDeviceSetup setup;
        const auto legacyName = xml.getStringAttribute ("audioDeviceName");
        if (legacyName.isNotEmpty())
        {
            setup.inputDeviceName = setup.outputDeviceName = legacyName;
        }
        else
        {
            setup.inputDeviceName = xml.getStringAttribute ("audioInputDeviceName");
            setup.outputDeviceName = xml.getStringAttribute ("audioOutputDeviceName");
        }

        setup.bufferSize = xml.getIntAttribute ("audioDeviceBufferSize", setup.bufferSize);
        setup.sampleRate = xml.getDoubleAttribute ("audioDeviceRate", setup.sampleRate);
        setup.inputChannels.parseString (xml.getStringAttribute ("audioDeviceInChans", "11"), 2);
        setup.outputChannels.parseString (xml.getStringAttribute ("audioDeviceOutChans", "11"), 2);
        setup.useDefaultInputChannels = ! xml.hasAttribute ("audioDeviceInChans");
        setup.useDefaultOutputChannels = ! xml.hasAttribute ("audioDeviceOutChans");

        const auto typeName = xml.getStringAttribute ("deviceType");
        if (typeName.isNotEmpty() && typeName != devices.getCurrentAudioDeviceType())
            devices.setCurrentAudioDeviceType (typeName, true);

        return devices.setAudioDeviceSetup (setup, true);
    }

    void closeDevice() override { context.devices().closeAudioDevice(); }

    std::unique_ptr<juce::XmlElement> stateXml() override
    {
        return context.devices().createStateXml();
    }

    void persist (const juce::XmlElement& xml) override
    {
        auto& settings = context.settings();
        if (auto* const props = settings.getUserSettings())
            props->setValue (Settings::devicesKey, &xml);
        settings.saveIfNeeded();
    }

    uint64_t ioTicks() override
    {
        if (auto engine = context.audio())
            return engine->ioCallbackTicks();
        return 0;
    }

    juce::String lastDeviceError() override
    {
        if (auto engine = context.audio())
            return engine->lastDeviceErrorMessage();
        return {};
    }

private:
    Context& context;
};

} // namespace

class DeviceService::Impl : private juce::Timer,
                            private juce::AsyncUpdater,
                            private juce::ChangeListener
{
public:
    Impl (DeviceService& o) : owner (o) {}
    ~Impl() { detach(); }

    void attach()
    {
        if (devices != nullptr)
            return;

        auto& context = owner.context();
        backend = std::make_unique<ContextBackend> (context);
        monitor = std::make_unique<AudioDeviceMonitor> (*backend);

        statusConnection = monitor->sigStatusChanged.connect (
            [this] (const AudioDeviceMonitor::Status& status) { owner.sigAudioDeviceStatus (status); });
        listChangedConnection = context.devices().sigDeviceListChanged.connect (
            [this]() { triggerAsyncUpdate(); });

        // Kept for detach: the device manager outlives the services in
        // Context teardown, but context() itself is not reachable there.
        devices = &context.devices();
        devices->addChangeListener (this);

        std::unique_ptr<juce::XmlElement> savedXml;
        if (auto* const props = context.settings().getUserSettings())
            savedXml = props->getXmlValue (Settings::devicesKey);
        monitor->seed (std::move (savedXml));

        startTimer (1000);
    }

    void detach()
    {
        stopTimer();
        cancelPendingUpdate();
        listChangedConnection.disconnect();
        statusConnection.disconnect();

        if (devices != nullptr)
        {
            devices->removeChangeListener (this);
            devices = nullptr;
        }

        monitor.reset();
        backend.reset();
    }

    AudioDeviceMonitor::Status status() const
    {
        return monitor != nullptr ? monitor->status() : AudioDeviceMonitor::Status();
    }

    void resumed()
    {
        if (monitor != nullptr)
            monitor->onResume();
    }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        // Defer: change messages can arrive while the device manager is
        // still inside its own callback stack.
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        if (monitor != nullptr)
            monitor->onChangeEvent();
    }

    void timerCallback() override
    {
        if (monitor != nullptr)
            monitor->onTimerTick();
    }

    DeviceService& owner;
    std::unique_ptr<ContextBackend> backend;
    std::unique_ptr<AudioDeviceMonitor> monitor;
    SignalConnection statusConnection, listChangedConnection;
    DeviceManager* devices { nullptr };
};

DeviceService::DeviceService()
{
    impl.reset (new Impl (*this));
}

DeviceService::~DeviceService()
{
    impl.reset (nullptr);
}

void DeviceService::activate()
{
    Service::activate();
    // Broadcast MIDI device hot-plug/removal to interested services (the callback
    // is delivered on the message thread).
    deviceListConnection = juce::MidiDeviceListConnection::make (
        [this] { sigMidiDevicesChanged(); });

    // Audio device monitoring only applies when Element owns the devices.
    if (getRunMode() != RunMode::Plugin)
        impl->attach();
}

void DeviceService::deactivate()
{
    impl->detach();
    deviceListConnection.reset();
    Service::deactivate();
}

AudioDeviceMonitor::Status DeviceService::audioDeviceStatus() const
{
    return impl->status();
}

void DeviceService::onResume()
{
    impl->resumed();
}

} // namespace element

// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include "services/devicemonitor.hpp"

using element::AudioDeviceMonitor;
using juce::String;
using juce::XmlElement;

namespace {

std::unique_ptr<XmlElement> makeSetup (const String& type, const String& input, const String& output)
{
    auto xml = std::make_unique<XmlElement> ("DEVICESETUP");
    xml->setAttribute ("deviceType", type);
    xml->setAttribute ("audioInputDeviceName", input);
    xml->setAttribute ("audioOutputDeviceName", output);
    return xml;
}

struct FakeBackend : public AudioDeviceMonitor::Backend
{
    AudioDeviceMonitor::DeviceInfo device;
    bool present { true };
    bool detectable { true };
    String openError;
    std::unique_ptr<XmlElement> state;
    std::unique_ptr<XmlElement> persisted;
    uint64_t ticks { 0 };
    String deviceError;

    int closeCalls { 0 };
    int openCalls { 0 };
    int persistCalls { 0 };
    int rescans { 0 };

    AudioDeviceMonitor::DeviceInfo currentDevice() override { return device; }

    bool isDevicePresent (const String&, const String&, bool rescan) override
    {
        if (rescan)
            ++rescans;
        return present;
    }

    bool presenceDetectable (const String&) override { return detectable; }

    String attemptOpenDesired (const XmlElement& xml) override
    {
        ++openCalls;
        if (openError.isNotEmpty())
            return openError;

        device.open = true;
        device.playing = true;
        device.typeName = xml.getStringAttribute ("deviceType");
        device.inputName = xml.getStringAttribute ("audioInputDeviceName");
        device.outputName = xml.getStringAttribute ("audioOutputDeviceName");
        state = std::make_unique<XmlElement> (xml);
        return {};
    }

    void closeDevice() override
    {
        ++closeCalls;
        device = {};
    }

    std::unique_ptr<XmlElement> stateXml() override
    {
        return state != nullptr ? std::make_unique<XmlElement> (*state) : nullptr;
    }

    void persist (const XmlElement& xml) override
    {
        ++persistCalls;
        persisted = std::make_unique<XmlElement> (xml);
    }

    uint64_t ioTicks() override { return ticks; }

    String lastDeviceError() override
    {
        auto message = deviceError;
        deviceError.clear();
        return message;
    }

    // Puts the fake in "device open and running" condition matching xml.
    void openFromSetup (const XmlElement& xml)
    {
        device.open = true;
        device.playing = true;
        device.typeName = xml.getStringAttribute ("deviceType");
        device.inputName = xml.getStringAttribute ("audioInputDeviceName");
        device.outputName = xml.getStringAttribute ("audioOutputDeviceName");
        state = std::make_unique<XmlElement> (xml);
    }
};

struct MonitorFixture
{
    FakeBackend backend;
    AudioDeviceMonitor monitor { backend };
    std::vector<AudioDeviceMonitor::Status> emitted;
    element::SignalConnection connection;

    MonitorFixture()
    {
        connection = monitor.sigStatusChanged.connect (
            [this] (const AudioDeviceMonitor::Status& status) { emitted.push_back (status); });
    }

    ~MonitorFixture() { connection.disconnect(); }

    void seedActive (const String& type = "TestType",
                     const String& input = "Test Device",
                     const String& output = "Test Device")
    {
        auto xml = makeSetup (type, input, output);
        backend.openFromSetup (*xml);
        monitor.seed (std::move (xml));
    }

    void makeWaiting()
    {
        seedActive();
        backend.present = false;
        monitor.onChangeEvent();
        BOOST_REQUIRE (monitor.status().state == AudioDeviceMonitor::State::waiting);
    }
};

} // namespace

BOOST_AUTO_TEST_SUITE (DeviceMonitorTests)

BOOST_FIXTURE_TEST_CASE (SeedWithOpenDeviceIsActive, MonitorFixture)
{
    seedActive();
    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::active);
    BOOST_CHECK_EQUAL (monitor.status().deviceName.toStdString(), "Test Device");
    BOOST_CHECK_EQUAL (backend.openCalls, 0);
    BOOST_CHECK_EQUAL (backend.closeCalls, 0);
    BOOST_CHECK_EQUAL (backend.persistCalls, 0);
}

BOOST_FIXTURE_TEST_CASE (SeedWithAbsentDeviceWaitsWithoutOpening, MonitorFixture)
{
    backend.present = false;
    monitor.seed (makeSetup ("TestType", "Gone Device", "Gone Device"));
    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::waiting);
    BOOST_CHECK_EQUAL (monitor.status().deviceName.toStdString(), "Gone Device");
    BOOST_CHECK_EQUAL (backend.openCalls, 0);
}

BOOST_FIXTURE_TEST_CASE (SeedWithoutSettingsIsInactive, MonitorFixture)
{
    monitor.seed (nullptr);
    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::inactive);
}

BOOST_FIXTURE_TEST_CASE (DisconnectClosesOnceAndWaits, MonitorFixture)
{
    seedActive();
    backend.present = false;
    monitor.onChangeEvent();

    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::waiting);
    BOOST_CHECK_EQUAL (backend.closeCalls, 1);
    BOOST_CHECK_EQUAL (backend.persistCalls, 0);

    // Repeated change events while gone don't close or open again.
    monitor.onChangeEvent();
    BOOST_CHECK_EQUAL (backend.closeCalls, 1);
    BOOST_CHECK_EQUAL (backend.openCalls, 0);
}

BOOST_FIXTURE_TEST_CASE (ReconnectRestoresDesiredDevice, MonitorFixture)
{
    makeWaiting();
    backend.present = true;
    monitor.onChangeEvent();

    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::active);
    BOOST_CHECK_EQUAL (backend.openCalls, 1);
    BOOST_CHECK (backend.device.open);
    // Reconnecting must not rewrite saved settings.
    BOOST_CHECK_EQUAL (backend.persistCalls, 0);
}

BOOST_FIXTURE_TEST_CASE (UserChangePersistsImmediately, MonitorFixture)
{
    seedActive();
    auto changed = makeSetup ("TestType", "Other Device", "Other Device");
    backend.openFromSetup (*changed);
    monitor.onChangeEvent();

    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::active);
    BOOST_CHECK_EQUAL (backend.persistCalls, 1);
    BOOST_REQUIRE (backend.persisted != nullptr);
    BOOST_CHECK_EQUAL (backend.persisted->getStringAttribute ("audioOutputDeviceName").toStdString(),
                       "Other Device");

    // The same settings again do not persist twice.
    monitor.onChangeEvent();
    BOOST_CHECK_EQUAL (backend.persistCalls, 1);
}

BOOST_FIXTURE_TEST_CASE (ForeignDeviceClosedWhileWaiting, MonitorFixture)
{
    makeWaiting();
    const auto closesBefore = backend.closeCalls;

    backend.device.open = true;
    backend.device.playing = true;
    backend.device.typeName = "TestType";
    backend.device.inputName = backend.device.outputName = "Wrong Device";

    monitor.onChangeEvent();
    BOOST_CHECK_EQUAL (backend.closeCalls, closesBefore + 1);
    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::waiting);
    BOOST_CHECK_EQUAL (backend.openCalls, 0);
}

BOOST_FIXTURE_TEST_CASE (WatchdogTripsWhenCallbacksFreeze, MonitorFixture)
{
    seedActive();
    backend.present = false; // hardware gone but no list event (e.g. ASIO)

    for (int i = 0; i < AudioDeviceMonitor::staleTicksBeforeConfirm; ++i)
        monitor.onTimerTick();

    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::waiting);
    BOOST_CHECK_EQUAL (backend.closeCalls, 1);
    BOOST_CHECK (backend.rescans > 0);
}

BOOST_FIXTURE_TEST_CASE (WatchdogResetsWhenCallbacksAdvance, MonitorFixture)
{
    seedActive();
    backend.present = false;

    for (int i = 0; i < AudioDeviceMonitor::staleTicksBeforeConfirm * 3; ++i)
    {
        backend.ticks += 100; // stream healthy
        monitor.onTimerTick();
    }

    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::active);
    BOOST_CHECK_EQUAL (backend.closeCalls, 0);
}

BOOST_FIXTURE_TEST_CASE (WatchdogForcesDisconnectDespiteStaleList, MonitorFixture)
{
    seedActive();
    backend.present = true; // stale list still claims the device exists

    for (int i = 0; i < AudioDeviceMonitor::staleTicksBeforeForce; ++i)
        monitor.onTimerTick();

    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::waiting);
    BOOST_CHECK_EQUAL (backend.closeCalls, 1);
}

BOOST_FIXTURE_TEST_CASE (DeviceErrorTripsWatchdogImmediately, MonitorFixture)
{
    seedActive();
    backend.present = false;
    backend.deviceError = "CoreAudio error";
    monitor.onTimerTick();

    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::waiting);
    BOOST_CHECK_EQUAL (backend.closeCalls, 1);
}

BOOST_FIXTURE_TEST_CASE (PollRestoresWhenDeviceReturns, MonitorFixture)
{
    makeWaiting();

    // Still gone: polls check presence but never open.
    for (int i = 0; i < AudioDeviceMonitor::reconnectPollTicks * 2; ++i)
        monitor.onTimerTick();
    BOOST_CHECK_EQUAL (backend.openCalls, 0);

    backend.present = true;
    for (int i = 0; i < AudioDeviceMonitor::reconnectPollTicks; ++i)
        monitor.onTimerTick();

    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::active);
    BOOST_CHECK_EQUAL (backend.openCalls, 1);
}

BOOST_FIXTURE_TEST_CASE (UndetectablePresencePollsByOpening, MonitorFixture)
{
    seedActive();
    backend.detectable = false; // ALSA-style: cached list can't be trusted
    backend.present = true;
    backend.openError = "device busy";

    // Force-disconnect via frozen callbacks.
    for (int i = 0; i < AudioDeviceMonitor::staleTicksBeforeForce; ++i)
        monitor.onTimerTick();
    BOOST_REQUIRE (monitor.status().state == AudioDeviceMonitor::State::waiting);

    // While the open fails, keep waiting and retrying.
    for (int i = 0; i < AudioDeviceMonitor::reconnectPollTicks; ++i)
        monitor.onTimerTick();
    BOOST_CHECK_EQUAL (backend.openCalls, 1);
    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::waiting);

    backend.openError.clear();
    for (int i = 0; i < AudioDeviceMonitor::reconnectPollTicks; ++i)
        monitor.onTimerTick();
    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::active);
    BOOST_CHECK_EQUAL (backend.openCalls, 2);
}

BOOST_FIXTURE_TEST_CASE (SelectingNoDeviceGoesInactive, MonitorFixture)
{
    seedActive();
    backend.device = {};
    backend.state = makeSetup ("TestType", "", "");
    monitor.onChangeEvent();

    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::inactive);

    // No reconnect attempts while inactive.
    for (int i = 0; i < AudioDeviceMonitor::reconnectPollTicks * 2; ++i)
        monitor.onTimerTick();
    BOOST_CHECK_EQUAL (backend.openCalls, 0);
}

BOOST_FIXTURE_TEST_CASE (SelectingDeviceWhileInactiveActivates, MonitorFixture)
{
    monitor.seed (nullptr);
    BOOST_REQUIRE (monitor.status().state == AudioDeviceMonitor::State::inactive);

    auto chosen = makeSetup ("TestType", "New Device", "New Device");
    backend.openFromSetup (*chosen);
    monitor.onChangeEvent();

    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::active);
    BOOST_CHECK_EQUAL (backend.persistCalls, 1);
}

BOOST_FIXTURE_TEST_CASE (ResumeRestoresWhileWaiting, MonitorFixture)
{
    makeWaiting();
    backend.present = true;
    monitor.onResume();

    BOOST_CHECK (monitor.status().state == AudioDeviceMonitor::State::active);
    BOOST_CHECK_EQUAL (backend.openCalls, 1);
}

BOOST_AUTO_TEST_SUITE_END()

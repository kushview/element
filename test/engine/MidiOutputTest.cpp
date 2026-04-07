// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "juce_audio_devices/juce_audio_devices.h"
#if JUCE_MAC
    #include <CoreMIDI/CoreMIDI.h>
    #include <boost/test/unit_test.hpp>
    #include "engine/midioutput.h"

using namespace element;

namespace {

struct LoopbackFixture {
    MIDIClientRef client {};
    MIDIEndpointRef dest {};
    MIDIPortRef outPort {};
    juce::CriticalSection lock;
    juce::Array<MIDIPacket> received;

    LoopbackFixture()
    {
        MIDIClientCreate (CFSTR ("LoopbackTestClient"), nullptr, nullptr, &client);

        MIDIDestinationCreate (client, CFSTR ("LoopbackDest"), [] (const MIDIPacketList* list, void* userData, void*) {
                auto& self = *static_cast<LoopbackFixture*> (userData);
                const MIDIPacket* packet = &list->packet[0];
                const juce::ScopedLock sl (self.lock);
                for (UInt32 i = 0; i < list->numPackets; ++i) {
                    self.received.add (*packet);
                    packet = MIDIPacketNext (packet);
                } }, this, &dest);

        MIDIOutputPortCreate (client, CFSTR ("LoopbackOut"), &outPort);
    }

    ~LoopbackFixture()
    {
        MIDIEndpointDispose (dest);
        MIDIPortDispose (outPort);
        MIDIClientDispose (client);
    }

    int waitForMessages (int count, int timeoutMs) const
    {
        auto start = juce::Time::getMillisecondCounter();
        for (;;) {
            {
                const juce::ScopedLock sl (lock);
                const int size = received.size();
                if (size > count) {
                    return size;
                }
            }
            if (juce::Time::getMillisecondCounter() - start > (juce::uint32) timeoutMs)
                return 0;
            juce::Thread::sleep (5);
        }
    }

    std::optional<MIDIPacket> getReceivedMessage()
    {
        const juce::ScopedLock sl (lock);
        if (received.size() > 0)
            return received.removeAndReturn (0);
        return std::nullopt;
    }
};

juce::MidiDeviceInfo findMidiOutputByName (const juce::String& name)
{
    for (auto& device : juce::MidiOutput::getAvailableDevices()) {
        if (device.name == name)
            return device;
    }

    return {};
}

} // namespace

BOOST_AUTO_TEST_SUITE (MidiOutputTests)
BOOST_AUTO_TEST_CASE (Instantiate)
{
    LoopbackFixture loopback;

    juce::MidiDeviceInfo loopbackDeviceInfo = findMidiOutputByName ("LoopbackDest");
    BOOST_CHECK_NE (loopbackDeviceInfo.identifier, "");

    std::unique_ptr<ElementMidiOutput> midiOutput;
    auto openResult = ElementMidiOutput::openDevice (loopbackDeviceInfo, midiOutput);
    BOOST_REQUIRE (openResult.wasOk());
    BOOST_REQUIRE (midiOutput != nullptr);

    const juce::MidiMessage& message1 = juce::MidiMessage::noteOn (1, 50, (juce::uint8) 100);
    const juce::MidiMessage& message2 = juce::MidiMessage::noteOff (1, 50, (juce::uint8) 100);
    juce::MidiBuffer buffer {};
    buffer.addEvent (message1, 0);

    constexpr double timeDelayMs = 5.0;
    constexpr double sampleRate = 44100.0;
    constexpr int sampleNumber = static_cast<int> (timeDelayMs * sampleRate / 1000.0);
    buffer.addEvent (message2, sampleNumber);
    auto result = midiOutput->sendBlockOfMessages (buffer, 0, sampleRate);

    BOOST_REQUIRE (result.wasOk());

    loopback.waitForMessages (2, 100);
    auto msg1 = loopback.getReceivedMessage();
    auto msg2 = loopback.getReceivedMessage();
    BOOST_REQUIRE (msg1.has_value());
    BOOST_REQUIRE (msg2.has_value());

    MIDITimeStamp delta = msg2->timeStamp - msg1->timeStamp;

    mach_timebase_info_data_t timebase;
    mach_timebase_info (&timebase);
    uint64_t deltaNanos = delta * timebase.numer / timebase.denom;

    double deltaMs = deltaNanos / 1'000'000.0;

    BOOST_CHECK_CLOSE (deltaMs, timeDelayMs, 20.0);
}
BOOST_AUTO_TEST_SUITE_END()
#endif
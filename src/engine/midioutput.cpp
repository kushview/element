// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "midioutput.h"
namespace element {
#if JUCE_MAC

MIDIEndpointRef findDestination (const juce::MidiDeviceInfo& deviceInfo)
{
    SInt32 targetUID = deviceInfo.identifier.containsChar (' ')
                           ? deviceInfo.identifier.fromLastOccurrenceOf (" ", false, false).getIntValue()
                           : deviceInfo.identifier.getIntValue();
    ItemCount count = MIDIGetNumberOfDestinations();

    for (ItemCount i = 0; i < count; ++i)
    {
        MIDIEndpointRef endpoint = MIDIGetDestination (i);
        SInt32 uid = 0;
        MIDIObjectGetIntegerProperty (endpoint, kMIDIPropertyUniqueID, &uid);

        if (uid == targetUID)
            return endpoint;
    }

    return 0;
}

ElementMidiOutput::ElementMidiOutput (const juce::MidiDeviceInfo& deviceInfo)
{
    destination = findDestination (deviceInfo);
    if (destination)
    {
        MIDIClientCreate (CFSTR ("ElementMIDIOutput"), nullptr, nullptr, &client);
        MIDIOutputPortCreate (client, deviceInfo.name.toCFString(), &port);
        mach_timebase_info (&timebase);
    }
}

juce::Result ElementMidiOutput::openDevice (const juce::MidiDeviceInfo& deviceInfo, std::unique_ptr<ElementMidiOutput>& out)
{
    out = std::make_unique<ElementMidiOutput> (deviceInfo);
    if (! out->destination)
    {
        out.reset();
        return juce::Result::fail ("MIDI destination not found: " + deviceInfo.name);
    }
    return juce::Result::ok();
}

void ElementMidiOutput::closeDevice()
{
    if (! destination)
        return;
    MIDIFlushOutput (destination);
    MIDIClientDispose (client);
    destination = 0;
}

MIDITimeStamp ElementMidiOutput::futureTimestamp (uint64_t nanosFromNow) const
{
    MIDITimeStamp ticks = nanosFromNow * timebase.denom / timebase.numer;
    return mach_absolute_time() + ticks;
}

juce::Result ElementMidiOutput::sendBlockOfMessages (const juce::MidiBuffer& midi, double delayMs, double sampleRate) const
{
    constexpr size_t BUFFER_SIZE_FOR_PACKET_LIST = 1024;
    uint8_t buffer[BUFFER_SIZE_FOR_PACKET_LIST];
    auto* packetList = reinterpret_cast<MIDIPacketList*> (buffer);
    MIDIPacket* packet = MIDIPacketListInit (packetList);

    for (juce::MidiMessageMetadata message : midi)
    {
        const double nanoSeconds = message.samplePosition / sampleRate * 1'000'000'000.0 + delayMs * 1'000'000.0;
        const auto ticks = futureTimestamp (static_cast<uint64_t> (std::round (nanoSeconds)));
        packet = MIDIPacketListAdd (packetList, BUFFER_SIZE_FOR_PACKET_LIST, packet, ticks, message.numBytes, message.data);
    }

    OSStatus status = MIDISend (port, destination, packetList);
    if (status != noErr)
        return juce::Result::fail ("MIDISend failed with OSStatus " + juce::String (status));
    return juce::Result::ok();
}

#else

void ElementMidiOutput::closeDevice()
{
    if (output)
    {
        output->stopBackgroundThread();
        output->clearAllPendingMessages();
        output.reset();
    }
}

ElementMidiOutput::ElementMidiOutput (const juce::MidiDeviceInfo& deviceInfo)
{
    output = juce::MidiOutput::openDevice (deviceInfo.identifier);

    if (output)
    {
        output->clearAllPendingMessages();
        output->startBackgroundThread();
    }
    else
    {
        DBG ("[element] could not open MIDI output: " << deviceInfo.name);
    }
}

juce::Result ElementMidiOutput::openDevice (const juce::MidiDeviceInfo& deviceInfo, std::unique_ptr<ElementMidiOutput>& out)
{
    out = std::make_unique<ElementMidiOutput> (deviceInfo);
    if (! out->output)
    {
        out.reset();
        return juce::Result::fail ("Could not open MIDI output: " + deviceInfo.name);
    }
    return juce::Result::ok();
}

juce::Result ElementMidiOutput::sendBlockOfMessages (const juce::MidiBuffer& midi, double delayMs, double sampleRate) const
{
#if JUCE_WINDOWS
    output->sendBlockOfMessages (
        midi, delayMs + juce::Time::getMillisecondCounter(), sampleRate);
#else
    output->sendBlockOfMessages (
        midi, delayMs + juce::Time::getMillisecondCounterHiRes(), sampleRate);
#endif
    return juce::Result::ok();
}
#endif

} // namespace element
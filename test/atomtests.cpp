// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <boost/test/unit_test.hpp>

#include <lv2/atom/util.h>
#include <lv2/midi/midi.h>
#include <lv2/time/time.h>
#include <lv2/urid/urid.h>

#include <lvtk/symbols.hpp>

#include <element/juce/audio_basics.hpp>
#include <element/atombuffer.hpp>

using AtomBuffer = element::AtomBuffer;
using MidiBuffer = juce::MidiBuffer;
using MidiMessage = juce::MidiMessage;

struct urids final {
    enum {
        none = 0,
        atom_eventTransfer,
        atom_Double,
        atom_Float,
        atom_Object,
        midi_MidiEvent,
        time_Position,
        time_speed,
        time_frame
    };

    static constexpr uint32_t begin() noexcept { return 1; };
    static constexpr uint32_t end() noexcept { return midi_MidiEvent + 1; }

    inline static auto makeMap()
    {
        lvtk::Symbols::map_type init;
        init[""] = none;
        for (uint32_t urid = urids::begin(); urid < urids::end(); ++urid)
            init[urids::uri (urid)] = urid;
        return init;
    }

    template <typename URID>
    inline static const char* uri (URID&& urid)
    {
        switch (static_cast<uint32_t> (urid)) {
            case atom_eventTransfer:
                return LV2_ATOM__eventTransfer;
                break;
            case atom_Double:
                return LV2_ATOM__Double;
                break;
            case atom_Float:
                return LV2_ATOM__Float;
                break;
            case atom_Object:
                return LV2_ATOM__Object;
                break;
            case midi_MidiEvent:
                return LV2_MIDI__MidiEvent;
                break;
            case time_Position:
                return LV2_TIME__Position;
                break;
            case time_speed:
                return LV2_TIME__speed;
                break;
            case time_frame:
                return LV2_TIME__frame;
                break;
            case none:
            default:
                return "";
                break;
        }
    }
};

static uint32_t writeTime (LV2_Atom_Forge& forge, AtomBuffer& port)
{
    uint8_t timeBuf[512] = { 0 };
    lv2_atom_forge_set_buffer (&forge, timeBuf, sizeof (timeBuf));
    LV2_Atom_Forge_Frame frame;
    auto ref = (LV2_Atom*) lv2_atom_forge_object (&forge, &frame, 0, urids::time_Position);

    lv2_atom_forge_key (&forge, urids::time_speed);
    lv2_atom_forge_float (&forge, 1.0f);

    lv2_atom_forge_key (&forge, urids::time_frame);
    lv2_atom_forge_long (&forge, 1234);

    lv2_atom_forge_pop (&forge, &frame);

    const auto total_size = ref->size;
    port.insert (0, ref->size, ref->type, LV2_ATOM_BODY (ref));
    return total_size;
}

BOOST_AUTO_TEST_SUITE (AtomTests)

BOOST_AUTO_TEST_CASE (Symbols_initializer)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);

    BOOST_REQUIRE_EQUAL (sym.map (LV2_MIDI__MidiEvent), urids::midi_MidiEvent);
    BOOST_REQUIRE_EQUAL (sym.map (LV2_ATOM__Object), urids::atom_Object);
    BOOST_REQUIRE_EQUAL (sym.unmap (urids::atom_Float), LV2_ATOM__Float);
    BOOST_REQUIRE_NE (sym.map (LV2_MIDI__MidiEvent), urids::atom_Object);
}

BOOST_AUTO_TEST_CASE (insert)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);

    AtomBuffer buffer;
    LV2_Atom_Forge forge;

    auto map = (LV2_URID_Map*) sym.map_feature()->data;
    lv2_atom_forge_init (&forge, map);
    buffer.setTypes (map);

    auto msg2 = MidiMessage::noteOn (1, 60, 0.6f);
    auto msg1 = MidiMessage::noteOff (1, 60);
    buffer.insert (100, msg1.getRawDataSize(), urids::midi_MidiEvent, msg1.getRawData());
    buffer.insert (50, msg2.getRawDataSize(), urids::midi_MidiEvent, msg2.getRawData());
    buffer.insert (1, msg1.getRawDataSize(), urids::midi_MidiEvent, msg1.getRawData());
    const auto timeSize = writeTime (forge, buffer);

    int index = 0;

    LV2_ATOM_SEQUENCE_FOREACH (buffer.sequence(), ev)
    {
        if (index == 0) {
            BOOST_REQUIRE_EQUAL (ev->body.type, urids::atom_Object);
            BOOST_REQUIRE_EQUAL (ev->body.size, timeSize);
            auto obj = (const LV2_Atom_Object*) &ev->body;
            BOOST_REQUIRE_EQUAL (obj->body.otype, urids::time_Position);
        } else if (index == 1) {
            // BOOST_REQUIRE_EQUAL (ev->body.type, urids::midi_MidiEvent);
            BOOST_REQUIRE_EQUAL (ev->body.size, 3U);
            BOOST_REQUIRE_EQUAL (1, ev->time.frames);
            MidiMessage t (LV2_ATOM_BODY (&ev->body), ev->body.size);
            BOOST_REQUIRE (t.isNoteOff());
            BOOST_REQUIRE_EQUAL (t.getNoteNumber(), 60);
            BOOST_REQUIRE_EQUAL (t.getChannel(), 1);
        } else if (index == 2) {
            BOOST_REQUIRE_EQUAL (ev->body.type, urids::midi_MidiEvent);
            BOOST_REQUIRE_EQUAL (ev->body.size, 3U);
            BOOST_REQUIRE_EQUAL (50, ev->time.frames);
            MidiMessage t (LV2_ATOM_BODY (&ev->body), ev->body.size);
            BOOST_REQUIRE (t.isNoteOn());
            BOOST_REQUIRE_EQUAL (t.getNoteNumber(), 60);
            BOOST_REQUIRE_EQUAL (t.getChannel(), 1);
        } else if (index == 3) {
            BOOST_REQUIRE_EQUAL (ev->body.type, urids::midi_MidiEvent);
            BOOST_REQUIRE_EQUAL (ev->body.size, 3U);
            BOOST_REQUIRE_EQUAL (100, ev->time.frames);
            MidiMessage t (LV2_ATOM_BODY (&ev->body), ev->body.size);
            BOOST_REQUIRE (t.isNoteOff());
            BOOST_REQUIRE_EQUAL (t.getNoteNumber(), 60);
            BOOST_REQUIRE_EQUAL (t.getChannel(), 1);
        }

        ++index;
    }

    BOOST_REQUIRE_EQUAL (index, 4);
}

BOOST_AUTO_TEST_CASE (swapping)
{
    juce::OwnedArray<AtomBuffer> array;
    {
        auto& b1 = *array.add (new AtomBuffer());
        const auto capacity = b1.capacity();

        float value = 1.f;
        b1.insert (0, sizeof (float), 0, &value);
        auto a1 = b1.atom();
        auto d1 = b1.data();

        auto& b2 = *array.add (new AtomBuffer());
        auto a2 = b2.atom();
        auto d2 = b2.data();

        BOOST_REQUIRE_EQUAL (b1.capacity(), b2.capacity());
        BOOST_REQUIRE_NE (d1, d2);

        BOOST_REQUIRE_EQUAL (a1, array[0]->atom());
        BOOST_REQUIRE_EQUAL (a2, array[1]->atom());

        b1.swap (b2);
        BOOST_REQUIRE_EQUAL (b1.atom(), a2);
        BOOST_REQUIRE_EQUAL (b2.atom(), a1);
        BOOST_REQUIRE_EQUAL (0, std::memcmp (b1.data(), d2, capacity));
        BOOST_REQUIRE_EQUAL (0, std::memcmp (b2.data(), d1, capacity));

        int count = 0;
        LV2_ATOM_SEQUENCE_FOREACH (b1.sequence(), ev)
        {
            ++count;
        }
        BOOST_REQUIRE_EQUAL (count, 0);

        count = 0;
        LV2_ATOM_SEQUENCE_FOREACH (b2.sequence(), ev)
        {
            ++count;
        }
        BOOST_REQUIRE_EQUAL (count, 1);

        b1.swap (b2);
        BOOST_REQUIRE_EQUAL (b1.atom(), a1);
        BOOST_REQUIRE_EQUAL (b2.atom(), a2);
        BOOST_REQUIRE_EQUAL (0, std::memcmp (b1.data(), d1, capacity));
        BOOST_REQUIRE_EQUAL (0, std::memcmp (b2.data(), d2, capacity));
    }

    array.clear (true);
}

BOOST_AUTO_TEST_CASE (pipe)
{
    juce::OwnedArray<AtomBuffer> array;
    {
        auto b1 = array.add (new AtomBuffer());
        auto b2 = array.add (new AtomBuffer());
        auto b3 = array.add (new AtomBuffer());

        juce::Array<int> channels = { 0, 2, 1 };
        element::AtomPipe pipe (array, channels);
        BOOST_REQUIRE_EQUAL (pipe.size(), array.size());
        BOOST_REQUIRE_EQUAL (pipe.writeBuffer (0), b1);
        BOOST_REQUIRE_EQUAL (pipe.writeBuffer (1), b3);
        BOOST_REQUIRE_EQUAL (pipe.writeBuffer (2), b2);
    }
    array.clear (true);
}

BOOST_AUTO_TEST_CASE (capacity_checking)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);
    auto map = (LV2_URID_Map*) sym.map_feature()->data;

    AtomBuffer buffer;
    buffer.setTypes (map);

    const auto initial_capacity = buffer.capacity();
    BOOST_REQUIRE_GT (initial_capacity, 8000U); // Should be ~8192, but aligned

    // Fill buffer near capacity with large events
    const size_t large_event_size = 1024;
    uint8_t large_data[large_event_size];
    memset (large_data, 0xAB, large_event_size);

    int events_inserted = 0;
    for (int frame = 0; frame < 1000; ++frame) {
        const auto size_before = buffer.sequence()->atom.size;
        buffer.insert (frame, large_event_size, urids::atom_Object, large_data);
        const auto size_after = buffer.sequence()->atom.size;

        if (size_after > size_before)
            ++events_inserted;
        else
            break; // Buffer full, insert silently failed
    }

    BOOST_REQUIRE_GT (events_inserted, 0);
    BOOST_REQUIRE_LT (events_inserted, 1000); // Should hit capacity before 1000 events

    // Verify silent failure on overflow
    const auto size_at_capacity = buffer.sequence()->atom.size;
    buffer.insert (9999, large_event_size, urids::atom_Object, large_data);
    BOOST_REQUIRE_EQUAL (buffer.sequence()->atom.size, size_at_capacity); // No change
}

BOOST_AUTO_TEST_CASE (clear_functionality)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);
    auto map = (LV2_URID_Map*) sym.map_feature()->data;

    AtomBuffer buffer;
    buffer.setTypes (map);

    // Add some events
    auto msg = MidiMessage::noteOn (1, 60, 0.8f);
    buffer.insert (0, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());
    buffer.insert (100, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());

    int count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (buffer.sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 2);

    // Clear and verify empty
    buffer.clear();
    BOOST_REQUIRE_EQUAL (buffer.sequence()->atom.size, sizeof (LV2_Atom_Sequence_Body));

    count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (buffer.sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 0);

    // Test clear(int, int) signature
    buffer.insert (50, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());
    buffer.clear (0, 512); // Should behave same as clear()
    count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (buffer.sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 0);
}

BOOST_AUTO_TEST_CASE (prepare_for_output)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);
    auto map = (LV2_URID_Map*) sym.map_feature()->data;

    AtomBuffer buffer;
    buffer.setTypes (map);

    // prepare() sets size to capacity - header
    buffer.prepare();
    const auto prepared_size = buffer.sequence()->atom.size;
    BOOST_REQUIRE_EQUAL (prepared_size, buffer.capacity() - sizeof (LV2_Atom_Sequence_Body));

    // After clear, size should be minimal
    buffer.clear();
    BOOST_REQUIRE_EQUAL (buffer.sequence()->atom.size, sizeof (LV2_Atom_Sequence_Body));
}

BOOST_AUTO_TEST_CASE (add_from_midi_buffer)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);
    auto map = (LV2_URID_Map*) sym.map_feature()->data;

    AtomBuffer buffer;
    buffer.setTypes (map);

    // Create JUCE MidiBuffer with events
    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.5f), 0);
    midi.addEvent (MidiMessage::noteOn (1, 64, 0.6f), 50);
    midi.addEvent (MidiMessage::noteOff (1, 60), 100);
    midi.addEvent (MidiMessage::noteOff (1, 64), 150);

    buffer.add (midi);

    int count = 0;
    int64_t prev_frame = -1;
    LV2_ATOM_SEQUENCE_FOREACH (buffer.sequence(), ev)
    {
        BOOST_REQUIRE_EQUAL (ev->body.type, urids::midi_MidiEvent);
        BOOST_REQUIRE_EQUAL (ev->body.size, 3U);
        BOOST_REQUIRE_GT (ev->time.frames, prev_frame); // Verify ordering
        prev_frame = ev->time.frames;
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 4);
}

BOOST_AUTO_TEST_CASE (add_from_atom_buffer)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);
    auto map = (LV2_URID_Map*) sym.map_feature()->data;

    AtomBuffer src, dst;
    src.setTypes (map);
    dst.setTypes (map);

    // Add events to source
    auto msg1 = MidiMessage::noteOn (1, 60, 0.7f);
    auto msg2 = MidiMessage::noteOff (1, 60);
    src.insert (10, msg1.getRawDataSize(), urids::midi_MidiEvent, msg1.getRawData());
    src.insert (50, msg2.getRawDataSize(), urids::midi_MidiEvent, msg2.getRawData());

    // Add source to destination
    dst.add (src);

    int count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (dst.sequence(), ev)
    {
        BOOST_REQUIRE_EQUAL (ev->body.type, urids::midi_MidiEvent);
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 2);

    // Add again to test merging
    dst.add (src);
    count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (dst.sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 4);
}

BOOST_AUTO_TEST_CASE (insert_midi_message_convenience)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);
    auto map = (LV2_URID_Map*) sym.map_feature()->data;

    AtomBuffer buffer;
    buffer.setTypes (map);

    auto msg1 = MidiMessage::noteOn (2, 72, 0.9f);
    auto msg2 = MidiMessage::controllerEvent (2, 7, 100);

    buffer.insert (msg1, 25);
    buffer.insert (msg2, 75);

    int count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (buffer.sequence(), ev)
    {
        BOOST_REQUIRE_EQUAL (ev->body.type, urids::midi_MidiEvent);

        if (count == 0) {
            BOOST_REQUIRE_EQUAL (ev->time.frames, 25);
            MidiMessage decoded (LV2_ATOM_BODY (&ev->body), ev->body.size);
            BOOST_REQUIRE (decoded.isNoteOn());
            BOOST_REQUIRE_EQUAL (decoded.getNoteNumber(), 72);
        } else if (count == 1) {
            BOOST_REQUIRE_EQUAL (ev->time.frames, 75);
            MidiMessage decoded (LV2_ATOM_BODY (&ev->body), ev->body.size);
            BOOST_REQUIRE (decoded.isController());
            BOOST_REQUIRE_EQUAL (decoded.getControllerNumber(), 7);
            BOOST_REQUIRE_EQUAL (decoded.getControllerValue(), 100);
        }

        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 2);
}

BOOST_AUTO_TEST_CASE (move_semantics)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);
    auto map = (LV2_URID_Map*) sym.map_feature()->data;

    AtomBuffer src;
    src.setTypes (map);

    auto msg = MidiMessage::noteOn (1, 60, 0.5f);
    src.insert (0, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());

    const auto src_data_ptr = src.data();
    const auto src_capacity = src.capacity();

    // Move construct
    AtomBuffer moved (std::move (src));
    BOOST_REQUIRE_EQUAL (moved.data(), src_data_ptr);
    BOOST_REQUIRE_EQUAL (moved.capacity(), src_capacity);

    int count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (moved.sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 1);

    // Move assign
    AtomBuffer assigned;
    const auto moved_data = moved.data();
    assigned = std::move (moved);
    BOOST_REQUIRE_EQUAL (assigned.data(), moved_data);

    count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (assigned.sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 1);
}

BOOST_AUTO_TEST_CASE (data_accessors)
{
    AtomBuffer buffer;

    const auto raw_ptr = buffer.data();
    const auto atom_ptr = buffer.atom();
    const auto seq_ptr = buffer.sequence();

    BOOST_REQUIRE_NE (raw_ptr, nullptr);
    BOOST_REQUIRE_NE (atom_ptr, nullptr);
    BOOST_REQUIRE_NE (seq_ptr, nullptr);
    BOOST_REQUIRE_EQUAL (raw_ptr, (const void*) atom_ptr);
    BOOST_REQUIRE_EQUAL (raw_ptr, (const void*) seq_ptr);

    // Test const version
    const AtomBuffer& const_ref = buffer;
    BOOST_REQUIRE_EQUAL (const_ref.data(), raw_ptr);
}

BOOST_AUTO_TEST_CASE (event_ordering)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);
    auto map = (LV2_URID_Map*) sym.map_feature()->data;

    AtomBuffer buffer;
    buffer.setTypes (map);

    // Insert events out of order
    auto msg = MidiMessage::noteOn (1, 60, 0.5f);
    buffer.insert (100, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());
    buffer.insert (200, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());
    buffer.insert (50, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());
    buffer.insert (150, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());
    buffer.insert (25, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());

    // Verify events are time-ordered
    int count = 0;
    int64_t expected_frames[] = { 25, 50, 100, 150, 200 };
    LV2_ATOM_SEQUENCE_FOREACH (buffer.sequence(), ev)
    {
        BOOST_REQUIRE_EQUAL (ev->time.frames, expected_frames[count]);
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 5);
}

BOOST_AUTO_TEST_CASE (mixed_event_types)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);
    auto map = (LV2_URID_Map*) sym.map_feature()->data;

    AtomBuffer buffer;
    LV2_Atom_Forge forge;
    lv2_atom_forge_init (&forge, map);
    buffer.setTypes (map);

    // Insert MIDI
    auto midi_msg = MidiMessage::noteOn (1, 60, 0.5f);
    buffer.insert (0, midi_msg.getRawDataSize(), urids::midi_MidiEvent, midi_msg.getRawData());

    // Insert Object (time position)
    writeTime (forge, buffer);

    // Insert more MIDI
    auto midi_msg2 = MidiMessage::noteOff (1, 60);
    buffer.insert (100, midi_msg2.getRawDataSize(), urids::midi_MidiEvent, midi_msg2.getRawData());

    // Insert Float atom
    float float_value = 3.14159f;
    buffer.insert (50, sizeof (float), urids::atom_Float, &float_value);

    int midi_count = 0, object_count = 0, float_count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (buffer.sequence(), ev)
    {
        if (ev->body.type == urids::midi_MidiEvent)
            ++midi_count;
        else if (ev->body.type == urids::atom_Object)
            ++object_count;
        else if (ev->body.type == urids::atom_Float)
            ++float_count;
    }

    BOOST_REQUIRE_EQUAL (midi_count, 2);
    BOOST_REQUIRE_EQUAL (object_count, 1);
    BOOST_REQUIRE_EQUAL (float_count, 1);
}

BOOST_AUTO_TEST_CASE (pipe_single_buffer)
{
    AtomBuffer buffer;
    element::AtomPipe pipe (buffer);

    BOOST_REQUIRE_EQUAL (pipe.size(), 1);
    BOOST_REQUIRE_EQUAL (pipe.writeBuffer (0), &buffer);
    BOOST_REQUIRE_EQUAL (pipe.readBuffer (0), &buffer);
}

BOOST_AUTO_TEST_CASE (pipe_array_constructor)
{
    AtomBuffer b1, b2, b3;
    AtomBuffer* buffers[] = { &b1, &b2, &b3 };

    element::AtomPipe pipe (buffers, 3);

    BOOST_REQUIRE_EQUAL (pipe.size(), 3);
    BOOST_REQUIRE_EQUAL (pipe.writeBuffer (0), &b1);
    BOOST_REQUIRE_EQUAL (pipe.writeBuffer (1), &b2);
    BOOST_REQUIRE_EQUAL (pipe.writeBuffer (2), &b3);
    BOOST_REQUIRE_EQUAL (pipe.readBuffer (0), &b1);
    BOOST_REQUIRE_EQUAL (pipe.readBuffer (1), &b2);
    BOOST_REQUIRE_EQUAL (pipe.readBuffer (2), &b3);
}

BOOST_AUTO_TEST_CASE (pipe_clear)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);
    auto map = (LV2_URID_Map*) sym.map_feature()->data;

    juce::OwnedArray<AtomBuffer> array;
    auto b1 = array.add (new AtomBuffer());
    auto b2 = array.add (new AtomBuffer());

    b1->setTypes (map);
    b2->setTypes (map);

    auto msg = MidiMessage::noteOn (1, 60, 0.5f);
    b1->insert (0, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());
    b2->insert (0, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());

    juce::Array<int> channels = { 0, 1 };
    element::AtomPipe pipe (array, channels);

    // Clear all buffers
    pipe.clear();

    int count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (b1->sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 0);

    count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (b2->sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 0);

    // Test clear with range (should behave same as clear())
    b1->insert (0, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());
    b2->insert (0, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());
    pipe.clear (0, 512);

    count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (b1->sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 0);

    count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (b2->sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 0);

    array.clear (true);
}

BOOST_AUTO_TEST_CASE (pipe_clear_single_channel)
{
    auto init = urids::makeMap();
    lvtk::Symbols sym (init);
    auto map = (LV2_URID_Map*) sym.map_feature()->data;

    juce::OwnedArray<AtomBuffer> array;
    auto b1 = array.add (new AtomBuffer());
    auto b2 = array.add (new AtomBuffer());

    b1->setTypes (map);
    b2->setTypes (map);

    auto msg = MidiMessage::noteOn (1, 60, 0.5f);
    b1->insert (0, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());
    b2->insert (0, msg.getRawDataSize(), urids::midi_MidiEvent, msg.getRawData());

    juce::Array<int> channels = { 0, 1 };
    element::AtomPipe pipe (array, channels);

    // Clear only channel 0
    pipe.clear (0, 0, 512);

    int count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (b1->sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 0);

    count = 0;
    LV2_ATOM_SEQUENCE_FOREACH (b2->sequence(), ev)
    {
        ++count;
    }
    BOOST_REQUIRE_EQUAL (count, 1); // b2 should still have event

    array.clear (true);
}

BOOST_AUTO_TEST_SUITE_END()

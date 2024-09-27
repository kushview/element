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

BOOST_AUTO_TEST_SUITE_END()

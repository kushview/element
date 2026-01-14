// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <cstring>

#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/midi/midi.h>
#include <lv2/atom/forge.h>
#include <lv2/urid/urid.h>

#include <element/aligneddata.hpp>
#include <element/datapipe.hpp>

namespace juce {
class MidiMessage;
class MidiBuffer;
} // namespace juce

namespace element {

class AtomBuffer final {
public:
    AtomBuffer();
    ~AtomBuffer();

    /** Set URID types from a URID map. */
    void setTypes (LV2_URID_Map* map);
    /** Set URID types directly. */
    void setTypes (uint32_t atomSequence, uint32_t midiEvent);

    /** Clear the buffer. */
    void clear();
    void clear (int, int) { clear(); }

    /** Prepare for connecting to an lv2:OutputPort, atom:AtomPort */
    void prepare();

    /** Insert event data at the given frame. */
    void insert (int64_t frames, uint32_t size, uint32_t type, const void* data);

    /** Insert a juce MidiMessage into the buffer. */
    void insert (juce::MidiMessage& msg, int frame);

    /** Add the contents of another atom buffer into this one. */
    void add (const AtomBuffer& other);

    /** Add the contents of a juce MidiBuffer into this one. */
    void add (juce::MidiBuffer& midi);

    /** Returns the total allocated memory. */
    inline constexpr uint32_t capacity() const noexcept { return _capacity; }

    /** Returns the underlying data. */
    inline constexpr void* data() noexcept { return _ptrs.raw; }
    /** Returns the underlying data. */
    inline constexpr const void* data() const noexcept { return _ptrs.raw; }

    /** Returns the atom used in this buffer. */
    inline constexpr const LV2_Atom* atom() const noexcept { return _ptrs.atom; }
    /** Returns the sequence used in this buffer. */
    inline constexpr const LV2_Atom_Sequence* sequence() const noexcept { return _ptrs.seq; }

    inline AtomBuffer (AtomBuffer&& o) noexcept
    {
        *this = std::move (o);
    }

    inline AtomBuffer& operator= (AtomBuffer&& o) noexcept
    {
        _data = std::move (o._data);
        _ptrs = std::move (o._ptrs);
        _capacity = std::move (o._capacity);
        MidiEvent = std::move (o.MidiEvent);
        return *this;
    }

    /** Swap two AtomBuffers. */
    inline void swap (AtomBuffer& b) noexcept
    {
        _data.swap (b._data);
        std::swap (_ptrs.raw, b._ptrs.raw);
        std::swap (_capacity, b._capacity);
        std::swap (MidiEvent, b.MidiEvent);
    }

private:
    AlignedData<8> _data;
    union {
        void* raw { nullptr };
        LV2_Atom* atom;
        LV2_Atom_Sequence* seq;
    } _ptrs;

    uint32_t _capacity { 0 };
    uint32_t MidiEvent { 0 };
};

using AtomPipe = DataPipe<AtomBuffer>;

} // namespace element

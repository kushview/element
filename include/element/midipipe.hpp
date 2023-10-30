// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/audio_basics.hpp>

struct lua_State;
namespace element {
namespace lua {
struct MidiBufferImpl;
}
} // namespace element

namespace element {

/** A glorified array of MidiBuffers used in rendering graph nodes */
class MidiPipe {
public:
    MidiPipe();
    MidiPipe (juce::MidiBuffer& buffer);
    MidiPipe (juce::MidiBuffer** buffers, int numBuffers);
    MidiPipe (const juce::OwnedArray<juce::MidiBuffer>& buffers, const juce::Array<int>& channels);
    ~MidiPipe();

    int getNumBuffers() const { return size; }
    const juce::MidiBuffer* const getReadBuffer (const int index) const;
    juce::MidiBuffer* const getWriteBuffer (const int index) const;

    void clear();
    void clear (int startSample, int numSamples);
    void clear (int index, int startSample, int numSamples);

private:
    enum {
        maxReferencedBuffers = 32
    };
    int size = 0;
    juce::MidiBuffer* referencedBuffers[maxReferencedBuffers];
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiPipe);
};

class LuaMidiPipe final {
public:
    LuaMidiPipe();
    ~LuaMidiPipe();
    /** Create a LuaMidiPipe and push to the Lua stack. 
        The returned object is owned by lua and will be deleted by
        the garbage collector.  Simply set to nullptr when not needed.
     */
    static LuaMidiPipe** create (lua_State* L, int numReserved);

    /** Returns the number of midi buffers contained */
    int getNumBuffers() const { return used; }

    /** Get a read only buffer */
    const juce::MidiBuffer* const getReadBuffer (int index) const;

    /** Get a writable buffer */
    juce::MidiBuffer* getWriteBuffer (int index) const;

    /** Change how many buffers are managed */
    void setSize (int newsize);

    /** Swap */
    void swapWith (MidiPipe&);

    /** Lua impls */
    static int get (lua_State* L);
    static int resize (lua_State* L);
    static int size (lua_State* L);
    static int clear (lua_State* L);

private:
    lua_State* state { nullptr };
    juce::Array<element::lua::MidiBufferImpl**> buffers;
    juce::Array<int> refs;
    int used { 0 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LuaMidiPipe);
};

} // namespace element

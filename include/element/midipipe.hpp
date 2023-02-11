/*
    This file is part of Element
    Copyright (C) 2019-2020  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

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
class MidiPipe
{
public:
    MidiPipe();
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
    enum
    {
        maxReferencedBuffers = 32
    };
    int size = 0;
    juce::MidiBuffer* referencedBuffers[maxReferencedBuffers];
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiPipe);
};

class LuaMidiPipe final
{
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

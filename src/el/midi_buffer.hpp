
#pragma once

#include <element/element.hpp>
#include "sol_helpers.hpp"
#include "JuceHeader.h"

namespace element {
namespace lua {

    struct MidiBufferImpl final
    {
        /** The buffer binding */
        juce::MidiBuffer buffer;
        /** Cached iterator to avoid allocating */
        juce::MidiBufferIterator iter;
        /** Cached message used by iterator */
        juce::MidiMessage** message { nullptr };
        int msgref { LUA_REFNIL };

        MidiBufferImpl (lua_State* L)
        {
            message = (juce::MidiMessage**) lua_newuserdata (L, sizeof (juce::MidiMessage**));
            *message = new juce::MidiMessage();
            luaL_setmetatable (L, EL_MT_MIDI_MESSAGE);
            msgref = luaL_ref (L, LUA_REGISTRYINDEX);
        }
        ~MidiBufferImpl() = default;

        void free (lua_State* L)
        {
            // garbage collector will free the data
            if (msgref != LUA_REFNIL)
            {
                msgref = LUA_REFNIL;
                luaL_unref (L, LUA_REGISTRYINDEX, msgref);
            }

            if (message != nullptr)
            {
                *message = nullptr;
                message = nullptr;
            }
        }

        void reset_iter()
        {
            iter = buffer.begin();
            **message = juce::MidiMessage();
        }
    };

    /** Allocate a new kv.MidiBuffer to the stack and set the metatable */
    inline static MidiBufferImpl**
        new_midibuffer (lua_State* L)
    {
        auto** impl = (MidiBufferImpl**) lua_newuserdata (L, sizeof (MidiBufferImpl**));
        *impl = new MidiBufferImpl (L);
        luaL_setmetatable (L, EL_MT_MIDI_BUFFER);
        return impl;
    }

} // namespace lua
} // namespace element

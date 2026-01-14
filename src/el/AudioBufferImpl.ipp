// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// An Audio buffer designed for real time performance.
// - **Safety**: For speed, virtually no type checking in method calls.
// - **Indexing**: Sample positions and channel numbers are 1-indexed.
// @classmod el.AudioBuffer

#if 1 // EL_LUA_AUDIO_BUFFER_COMPILE

#include <element/element.h>
#include <element/juce/audio_basics.hpp>

#include "sol_helpers.hpp"

#ifndef EL_LUA_AUDIO_BUFFER_32
#define EL_LUA_AUDIO_BUFFER_32 0
#endif

#if EL_LUA_AUDIO_BUFFER_32
#define EL_MT_AUDIO_BUFFER_TYPE "el.AudioBuffer32Class"
#define EL_MT_AUDIO_BUFFER_IMPL EL_MT_AUDIO_BUFFER_32
#define EL_LUA_AUDIO_BUFFER_OPEN luaopen_el_AudioBuffer32
using SampleType = float;

#else
#define EL_MT_AUDIO_BUFFER_TYPE "el.AudioBuffer64Class"
#define EL_MT_AUDIO_BUFFER_IMPL EL_MT_AUDIO_BUFFER_64
#define EL_LUA_AUDIO_BUFFER_OPEN luaopen_el_AudioBuffer
using SampleType = lua_Number;
#endif

using Buffer = juce::AudioBuffer<SampleType>;

#define toclassref(L, n) *(Buffer**) lua_touserdata (L, n);

static int audio_isfloat (lua_State* L)
{
#if EL_LUA_AUDIO_BUFFER_32
    lua_pushboolean (L, true);
#else
    lua_pushboolean (L, false);
#endif
    return 1;
}

static int audio_isdouble (lua_State* L)
{
#if EL_LUA_AUDIO_BUFFER_32
    lua_pushboolean (L, false);
#else
    lua_pushboolean (L, true);
#endif
    return 1;
}

static int audio_channels (lua_State* L)
{
    auto* buf = toclassref (L, 1);
    lua_pushinteger (L, static_cast<lua_Integer> (buf->getNumChannels()));
    return 1;
}

static int audio_clear (lua_State* L)
{
    auto* buf = toclassref (L, 1);

    switch (lua_gettop (L))
    {
        case 2: {
            buf->clear (static_cast<int> (lua_tointeger (L, 2) - 1),
                        0,
                        buf->getNumSamples());
            break;
        }

        case 3: {
            buf->clear (static_cast<int> (lua_tointeger (L, 2)),
                        static_cast<int> (lua_tointeger (L, 3)));
            break;
        }

        case 4: {
            buf->clear (static_cast<int> (lua_tointeger (L, 2) - 1),
                        static_cast<int> (lua_tointeger (L, 3) - 1),
                        static_cast<int> (lua_tointeger (L, 4)));
            break;
        }

        default: {
            buf->clear();
            break;
        }
    }

    return 0;
}

static int audio_cleared (lua_State* L)
{
    auto* buf = toclassref (L, 1);
    lua_pushboolean (L, (int) buf->hasBeenCleared());
    return 1;
}

static int audio_length (lua_State* L)
{
    auto* buf = toclassref (L, 1);
    lua_pushinteger (L, static_cast<lua_Integer> (buf->getNumSamples()));
    return 1;
}

static int audio_get (lua_State* L)
{
    // clang-format off
    auto* buf = toclassref (L, 1);
    lua_pushnumber (L, buf->getArrayOfReadPointers()
        [lua_tointeger (L, 2) - 1]
        [lua_tointeger (L, 3) - 1]);
    return 1;
    // clang-format on
}

static int audio_set (lua_State* L)
{
    auto* buf = toclassref (L, 1);
    buf->getArrayOfWritePointers()
        [lua_tointeger (L, 2) - 1]
        [lua_tointeger (L, 3) - 1] = static_cast<SampleType> (lua_tonumber (L, 4));
    return 0;
}

static int audio_applygain (lua_State* L)
{
    Buffer* buf = toclassref (L, 1);
    switch (lua_gettop (L))
    {
        case 2: {
            buf->applyGain (static_cast<SampleType> (lua_tonumber (L, 2)));
            break;
        }

        case 3: {
            buf->applyGain (static_cast<int> (lua_tointeger (L, 2) - 1),
                            0,
                            buf->getNumSamples(),
                            static_cast<SampleType> (lua_tonumber (L, 3)));
            break;
        }

        case 4: {
            buf->applyGain (static_cast<int> (lua_tointeger (L, 2) - 1),
                            static_cast<int> (lua_tointeger (L, 3)),
                            static_cast<SampleType> (lua_tonumber (L, 3)));
            break;
        }

        case 5: {
            buf->applyGain (
                static_cast<int> (lua_tointeger (L, 2)) - 1,
                static_cast<int> (lua_tointeger (L, 3)) - 1,
                static_cast<int> (lua_tointeger (L, 4)),
                static_cast<SampleType> (lua_tonumber (L, 5)));
            break;
        }
    }

    return 0;
}

static int audio_fade (lua_State* L)
{
    auto* buf = toclassref (L, 1);
    switch (lua_gettop (L))
    {
        case 3: {
            // apply gain to all channels/frames
            buf->applyGainRamp (0,
                                buf->getNumSamples(),
                                static_cast<SampleType> (lua_tonumber (L, 2)),
                                static_cast<SampleType> (lua_tonumber (L, 3)));
            break;
        }

        case 6: {
            // apply fade to specific channel with start and frame count
            buf->applyGainRamp (static_cast<int> (lua_tointeger (L, 2) - 1),
                                static_cast<int> (lua_tointeger (L, 3) - 1),
                                static_cast<int> (lua_tointeger (L, 4)),
                                static_cast<SampleType> (lua_tonumber (L, 5)),
                                static_cast<SampleType> (lua_tonumber (L, 6)));
            break;
        }
    }

    return 0;
}

static int audio_free (lua_State* L)
{
    auto** buf = (Buffer**) lua_touserdata (L, 1);
    if (nullptr != *buf)
    {
        delete (*buf);
        *buf = nullptr;
    }
    return 0;
}

static int audio_tostring (lua_State* L)
{
    auto* buf = toclassref (L, 1);
    const auto str = element::lua::to_string (*buf, "AudioBuffer");
    lua_pushstring (L, str.c_str());
    return 1;
}

/// Creates a empty audio buffer.
// @function AudioBuffer.new
// @return An empty buffer.
// @within Constructors
// @usage
// local buf = AudioBuffer.new()
// -- do someting with `buf`

/// Creates a new audio buffer with the specified channel and sample counts
// @int nchannels Number of channels
// @int nframes Number of samples in each channel
// @function AudioBuffer.new
// @return Newly created audio buffer
// @within Constructors
// @usage
// -- 2 channels, 2048 samples
// local buf = AudioBuffer.new (2, 2048)
// -- do someting with `buf`
static int audio_new (lua_State* L)
{
    auto** buf = (Buffer**) lua_newuserdata (L, sizeof (Buffer**));

    int nchans = 0, nframes = 0;
    if (lua_gettop (L) >= 2 && lua_isinteger (L, 1) && lua_isinteger (L, 2))
    {
        nchans = (int) juce::jmax (lua_Integer(), lua_tointeger (L, 1));
        nframes = (int) juce::jmax (lua_Integer(), lua_tointeger (L, 2));
    }

    *buf = new Buffer (nchans, nframes);
    luaL_setmetatable (L, EL_MT_AUDIO_BUFFER_IMPL);
    return 1;
}

//==============================================================================
static const luaL_Reg buffer_methods[] = {
    { "__gc", audio_free },
    { "__tostring", audio_tostring },

    /// Free used memory.
    // Invoke this to free the buffer when it is no longer needed.  Once called,
    // the buffer is no longer valid and WILL crash the interpreter if used after
    // the fact. Only use this when you don't want to rely on the garbage collector
    // to free memory.
    // @function AudioBuffer:free
    { "free", audio_free },

    /// Returns true if audio data is 32bit float.
    // Call this to find out audio data precision.
    // @function AudioBuffer:isFloat
    { "isFloat", audio_isfloat },

    /// Returns true if audio data is 64bit float.
    // Call this to find out audio data precision.
    // @function AudioBuffer:isDouble
    { "isDouble", audio_isdouble },

    /// Channel count
    // @function AudioBuffer:channels
    // @return the number of channels held in the buffer
    { "channels", audio_channels },

    /// returns the number of samples in the buffer
    // @function AudioBuffer:length
    // @return size of the buffer in samples
    { "length", audio_length },

    /// Clears the entire audio buffer.
    // @function AudioBuffer:clear

    /// Clears one channel in the audio buffer.
    // @int channel The channel to clear
    // @function AudioBuffer:clear

    /// Clears a range of samples on all channels.
    // @int start Sample index to start on
    // @int count Number of samples to clear
    // @function AudioBuffer:clear

    /// Clears a range of samples on one channel
    // @int channel The channel to clear
    // @int start Sample index to start on
    // @int count Number of samples to clear
    // @function AudioBuffer:clear
    { "clear", audio_clear },

    /// Returns true if the buffer has been cleared
    // @function AudioBuffer:cleared
    // @return true if already cleared
    { "cleared", audio_cleared },

    /// Get a single value from the buffer
    // @param channel The channel to get from
    // @param frame The sample index to get
    // @function AudioBuffer:get
    // @return the number of channels held in the buffer
    { "get", audio_get },

    /// Set a single sample in the buffer
    // @param channel The channel to set on
    // @param frame The frame index to set
    // @param value The value to set
    // @function AudioBuffer:set
    { "set", audio_set },

    /// Apply gain to all channels and samples
    // @number gain Gain to apply to all channels
    // @function AudioBuffer:applyGain

    /// Apply gain to a single channel
    // @int channel Channel to apply gain to
    // @number gain Gain to apply to a channel
    // @function AudioBuffer:applyGain

    /// Applies a gain multiple to a region of all the channels.
    // For speed, this doesn't check whether the sample numbers
    // are in-range, so be careful!
    // @int start Sample index to start at
    // @int count Number of samples to process
    // @number gain Amount of gain to apply
    // @function AudioBuffer:applyGain

    /// Apply gain to a single channel with range
    // @int channel Channel to apply gain to
    // @int start Sample index to start at
    // @int count Number of samples to process
    // @number gain Amount of gain to apply
    // @function AudioBuffer:applyGain
    { "applyGain", audio_applygain },

    /// Fade the buffer from starting gain to ending gain
    // @number startgain Starting gain
    // @number endgain End gain
    // @function AudioBuffer:fade

    /// Fade the buffer from starting gain to ending gain
    // @int channel Channel to apply to
    // @int start Sample to start at
    // @int count Number of samples to process
    // @number gain1 Starting gain
    // @number gain2 End gain
    // @function AudioBuffer:fade
    { "fade", audio_fade },
    { NULL, NULL }
};

//==============================================================================
#if EL_LUA_AUDIO_BUFFER_32
EL_PLUGIN_EXPORT
int luaopen_el_AudioBuffer32 (lua_State* L)
{
#else
EL_PLUGIN_EXPORT
int luaopen_el_AudioBuffer64 (lua_State* L)
{
#endif
    if (luaL_newmetatable (L, EL_MT_AUDIO_BUFFER_IMPL))
    {
        lua_pushvalue (L, -1); /* duplicate the metatable */
        lua_setfield (L, -2, "__index"); /* mt.__index = mt */
        luaL_setfuncs (L, buffer_methods, 0);
        lua_pop (L, 1);
    }

    if (luaL_newmetatable (L, EL_MT_AUDIO_BUFFER_TYPE))
    {
        lua_pop (L, 1);
    }

    lua_newtable (L);
    luaL_setmetatable (L, EL_MT_AUDIO_BUFFER_TYPE);
    lua_pushcfunction (L, audio_new);
    lua_setfield (L, -2, "new");
    return 1;
}

#endif

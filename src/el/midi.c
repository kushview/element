// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// MIDI factories and utilities.
// Functions in this module **do not** check arguments. It is the responsibility
// of calling code to ensure correct data is passed.
// @author Michael Fisher
// @module el.midi

#include <math.h>

#include "element/element.h"
#include <lauxlib.h>

typedef union _PackedMessage
{
    int64_t packed;
    struct
    {
        uint8_t byte1;
        uint8_t byte2;
        uint8_t byte3;
        uint8_t byte4;
    } data;
} PackedMessage;

/// Messages
// @section messages

static int f_msg3bytes (lua_State* L, uint8_t status)
{
    PackedMessage msg = { .packed = 0x00 };
    msg.data.byte1 = status | (uint8_t) (lua_tointeger (L, 1) - 1);
    msg.data.byte2 = (uint8_t) lua_tointeger (L, 2);
    msg.data.byte3 = (uint8_t) lua_tointeger (L, 3);
    msg.data.byte4 = 0x00;
    lua_pushinteger (L, msg.packed);
    return 1;
}

/// Make a controller message
// @function controller
// @int channel MIDI Channel
// @int controller Controller number
// @int value Controller Value
// @return MIDI message packed as Integer
// @within Messages
static int f_controller (lua_State* L)
{
    return f_msg3bytes (L, 0xb0);
}

/// Make a note on message
// @function noteon
// @int channel MIDI Channel
// @int note Note number 0-127
// @int velocity Note velocity 0-127
// @return MIDI message packed as Integer
// @within Messages
static int f_noteon (lua_State* L)
{
    return f_msg3bytes (L, 0x90);
}

/// Make a regular note off message
// @function noteoff
// @int channel MIDI Channel
// @int note Note number
// @return MIDI message packed as Integer
// @within Messages

/// Make a note off message with velocity
// @function noteoff
// @int channel MIDI Channel
// @int note Note number
// @int velocity Note velocity
// @return MIDI message packed as Integer
// @within Messages
static int f_noteoff (lua_State* L)
{
    if (lua_gettop (L) == 2)
        lua_pushinteger (L, 0x00);
    return f_msg3bytes (L, 0x80);
}

/// Make a program change message.
// @function program
// @int channel The midi channel (1 to 16)
// @int program Program number
// @return MIDI message packed as Integer
// @within Messages
static int f_program (lua_State* L)
{
    lua_pushinteger (L, 0x00);
    return f_msg3bytes (L, 0xC0);
}

/// Make a pitch wheel message.
// @function pitch
// @int channel The midi channel (1 to 16)
// @int position The wheel position, in the range 0 to 16383
// @return MIDI message packed as Integer
// @within Messages
static int f_pitch (lua_State* L)
{
    lua_Integer position = lua_tointeger (L, 2);
    PackedMessage msg = { .packed = 0x00 };
    msg.data.byte1 = 0xE0 | (uint8_t) (lua_tointeger (L, 1) - 1);
    msg.data.byte2 = (uint8_t) (position & 127);
    msg.data.byte3 = (uint8_t) ((position >> 7) & 127);
    lua_pushinteger (L, msg.packed);
    return 1;
}

/// Make an after touch message.
// @function aftertouch
// @int channel The midi channel (1 to 16)
// @int note The midi note, in the range 0 to 127
// @int value The after touch value.
// @return MIDI message packed as Integer
// @within Messages
static int f_aftertouch (lua_State* L)
{
    return f_msg3bytes (L, 0xA0);
}

/// Make a channel-pressure message.
// @function channelpressure
// @int channel The midi channel (1 to 16)
// @int value The pressure value (0 to 127)
// @return MIDI message packed as Integer
// @within Messages
static int f_channelpressure (lua_State* L)
{
    lua_pushinteger (L, 0x00);
    return f_msg3bytes (L, 0xD0);
}

/// Make an all notes off message.
// @function allnotesoff
// @int channel The midi channel (1 to 16)
// @return MIDI message packed as Integer
// @within Messages
static int f_allnotesoff (lua_State* L)
{
    lua_pushinteger (L, 123);
    lua_pushinteger (L, 0);
    return f_controller (L);
}

/// Make an all sounds off message.
// @function allsoundsoff
// @int channel   The midi channel (1 to 16)
// @return MIDI message packed as Integer
// @within Messages
static int f_allsoundsoff (lua_State* L)
{
    lua_pushinteger (L, 120);
    lua_pushinteger (L, 0);
    return f_controller (L);
}

/// Make an all controllers off message.
// @function allcontrollersoff
// @int channel   The midi channel (1 to 16)
// @return MIDI message packed as Integer
// @within Messages
static int f_allcontrollersoff (lua_State* L)
{
    lua_pushinteger (L, 121);
    lua_pushinteger (L, 0);
    return f_controller (L);
}

/// Make a clock message.
// @function clock
// @return MIDI message packed as Integer
// @within Messages
static int f_clock (lua_State* L)
{
    PackedMessage msg = { .packed = 0x00 };
    msg.data.byte1 = 0xF8;
    lua_pushinteger (L, msg.packed);
    return 1;
}

/// Make a start message.
// @function start
// @return MIDI message packed as Integer
// @within Messages
static int f_start (lua_State* L)
{
    PackedMessage msg = { .packed = 0x00 };
    msg.data.byte1 = 0xFA;
    lua_pushinteger (L, msg.packed);
    return 1;
}

/// Make a stop message.
// @function stop
// @return MIDI message packed as Integer
// @within Messages
static int f_stop (lua_State* L)
{
    PackedMessage msg = { .packed = 0x00 };
    msg.data.byte1 = 0xFC;
    lua_pushinteger (L, msg.packed);
    return 1;
}

/// Make a continue message.
// @function continue
// @return MIDI message packed as Integer
// @within Messages
static int f_continue (lua_State* L)
{
    PackedMessage msg = { .packed = 0x00 };
    msg.data.byte1 = 0xFB;
    lua_pushinteger (L, msg.packed);
    return 1;
}

/// Convert a MIDI note to hertz.
// This version assumes A 440 Hz
// @function tohertz
// @int note Note number
// @return Value in hertz
// @within Utils
static int f_tohertz (lua_State* L)
{
    lua_Number value = 440.0 * pow (2.0, (lua_tointeger (L, 1) - 69) / 12.0);
    lua_pushnumber (L, value);
    return 1;
}

/// Clamp to a valid MIDI value (0 - 127)
// @function clamp
// @int value The value to clamp
// @return The clamped value.
// @within Utils
static int f_clamp (lua_State* L)
{
    lua_Integer value = lua_tointeger (L, 1);
    if (value < 0)
        value = 0;
    else if (value > 127)
        value = 127;
    lua_pushinteger (L, value);
    return 1;
}

static const luaL_Reg midi_f[] = {
    { "controller", f_controller },
    { "noteon", f_noteon },
    { "noteoff", f_noteoff },
    { "program", f_program },
    { "pitch", f_pitch },

    { "aftertouch", f_aftertouch },
    { "channelpressure", f_channelpressure },

    { "allnotesoff", f_allnotesoff },
    { "allsoundsoff", f_allsoundsoff },
    { "allcontrollersoff", f_allcontrollersoff },

    { "clock", f_clock },
    { "start", f_start },
    { "stop", f_stop },
    { "continue", f_continue },

    { "tohertz", f_tohertz },
    { "clamp", f_clamp },
    { NULL, NULL }
};

EL_PLUGIN_EXPORT
int luaopen_el_midi (lua_State* L)
{
    luaL_newlib (L, midi_f);
    return 1;
}

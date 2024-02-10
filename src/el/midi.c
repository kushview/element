// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

/// MIDI utilities.
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
// @function programchange
// @int channel MIDI Channel
// @int program Program number
// @return MIDI message packed as Integer
// @within Messages
static int f_programchange (lua_State* L)
{
    lua_pushinteger (L, 0x00);
    return f_msg3bytes (L, 0xC0);
}

/// Convert a MIDI note to hertz.
// This version assumes A 440 Hz
// @function tohertz
// @int note Note number
// @return Value in hertz
// @within Utils
static int f_tohertz (lua_State* L)
{
    double value = 440.0 * pow (2.0, (lua_tointeger (L, 1) - 69) / 12.0);
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
    return 0;
}

static const luaL_Reg midi_f[] = {
    { "controller", f_controller },
    { "noteon", f_noteon },
    { "noteoff", f_noteoff },
    { "programchange", f_programchange },
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

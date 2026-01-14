// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// A MIDI Message.
// @classmod el.MidiMessage
// @pragma nostrip

#include <element/element.h>
#include <element/juce/audio_basics.hpp>

#include "sol_helpers.hpp"
#include "packed.h"

#define EL_MT_MIDI_MESSAGE_TYPE "el.MidiMessageClass"

static auto create_message (lua_State* L)
{
    auto** userdata = (juce::MidiMessage**) lua_newuserdata (L, sizeof (juce::MidiMessage**));
    *userdata = new juce::MidiMessage();
    luaL_setmetatable (L, EL_MT_MIDI_MESSAGE);
    return userdata;
}

/// Constructors.
// @section ctors

/// Create a new MIDI message.
// @function MidiMessage.new
// @int data Data as a packed integer. see @{el.midi}
// @treturn el.MidiMessage
static int midimessage_new (lua_State* L)
{
    auto** msg = create_message (L);
    if (lua_gettop (L) >= 1 && lua_isinteger (L, 1))
    {
        kv_packed_t pack;
        pack.packed = lua_tointeger (L, 1);
        **msg = juce::MidiMessage (pack.data[0], pack.data[1], pack.data[2]);
    }
    return 1;
}

static int midimessage_free (lua_State* L)
{
    auto** userdata = (juce::MidiMessage**) lua_touserdata (L, 1);
    if (nullptr != *userdata)
    {
        delete (*userdata);
        *userdata = nullptr;
    }
    return 0;
}

#define midimessage_get_string(f, m)                              \
    static int midimessage_##f (lua_State* L)                     \
    {                                                             \
        auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
        lua_pushstring (L, msg->m().toRawUTF8());                 \
        return 1;                                                 \
    }

#define midimessage_get_number(f, m)                              \
    static int midimessage_##f (lua_State* L)                     \
    {                                                             \
        auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
        lua_pushnumber (L, static_cast<lua_Number> (msg->m()));   \
        return 1;                                                 \
    }

#define midimessage_set_float(f, m)                               \
    static int midimessage_##f (lua_State* L)                     \
    {                                                             \
        auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
        msg->m (static_cast<lua_Number> (lua_tonumber (L, 2)));   \
        return 0;                                                 \
    }

#define midimessage_get_int(f, m)                                 \
    static int midimessage_##f (lua_State* L)                     \
    {                                                             \
        auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
        lua_pushinteger (L, msg->m());                            \
        return 1;                                                 \
    }

#define midimessage_set_int(f, m)                                 \
    static int midimessage_##f (lua_State* L)                     \
    {                                                             \
        auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
        msg->m (static_cast<int> (lua_tointeger (L, 2)));         \
        return 0;                                                 \
    }

#define midimessage_is(f, m)                                      \
    static int midimessage_is_##f (lua_State* L)                  \
    {                                                             \
        auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1); \
        lua_pushboolean (L, msg->m());                            \
        return 1;                                                 \
    }

// clang-format off
midimessage_get_string (description, getDescription)

midimessage_get_number (time, getTimeStamp)
midimessage_set_float (set_time, setTimeStamp)
midimessage_set_float (add_time, addToTimeStamp)

static int midimessage_with_time (lua_State* L)
{
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    auto** ret = create_message (L);
    (**ret) = (*msg);
    (**ret).setTimeStamp (lua_tonumber (L, 2));
    return 1;
}

midimessage_get_int (channel, getChannel)
midimessage_set_int (set_channel, setChannel) 

static int midimessage_isforchannel (lua_State* L)
{
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushboolean (L, msg->isForChannel (static_cast<int> (lua_tointeger (L, 2))));
    return 1;
}

midimessage_is (note_on, isNoteOn)
midimessage_is (note_off, isNoteOff)
midimessage_is (note, isNoteOnOrOff)
midimessage_get_int (note, getNoteNumber)
midimessage_set_int (set_note, setNoteNumber)

static int midimessage_data (lua_State* L)
{
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushlightuserdata (L, (void*) msg->getRawData());
    lua_pushinteger (L, msg->getRawDataSize());
    return 2;
}

midimessage_is (sysex, isSysEx)

static int midimessage_sysExData (lua_State* L)
{
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushlightuserdata (L, (void*) msg->getSysExData());
    lua_pushinteger (L, msg->getSysExDataSize());
    return 2;
}

midimessage_get_int (velocity, getVelocity)
midimessage_get_number (velocity_float, getFloatVelocity)
midimessage_set_float (set_velocity, setVelocity)
midimessage_set_float (multiply_velocity, multiplyVelocity)

midimessage_is (program, isProgramChange)
midimessage_get_int (program, getProgramChangeNumber)

midimessage_is (pitch, isPitchWheel)
midimessage_get_int (pitch, getPitchWheelValue)

midimessage_is (aftertouch, isAftertouch)
midimessage_get_int (aftertouch, getAfterTouchValue)

midimessage_is (pressure, isChannelPressure)
midimessage_get_int (pressure, getChannelPressureValue)

midimessage_is (controller, isController)
midimessage_get_int (controller, getControllerNumber)
midimessage_get_int (controller_value, getControllerValue)

static int midimessage_is_controller_type (lua_State* L)
{
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushboolean (L, msg->isControllerOfType (static_cast<int> (lua_tointeger (L, 2))));
    return 1;
}

midimessage_is (notes_off, isAllNotesOff)
midimessage_is (sound_off, isAllSoundOff)
midimessage_is (reset_controllers, isResetAllControllers)

midimessage_is (meta, isMetaEvent)
midimessage_get_int (meta_type, getMetaEventType)
midimessage_get_int (meta_length, getMetaEventLength)

static int midimessage_meta_data (lua_State* L)
{
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushlightuserdata (L, (void*) msg->getMetaEventData());
    return 1;
}

midimessage_is (track, isTrackMetaEvent)
midimessage_is (end_of_track, isEndOfTrackMetaEvent)
midimessage_is (track_name, isTrackNameEvent)

midimessage_is (text, isTextMetaEvent)
midimessage_get_string (text, getTextFromTextMetaEvent)

midimessage_is (tempo, isTempoMetaEvent)
midimessage_get_number (tempo_seconds_pqn, getTempoSecondsPerQuarterNote)

static int midimessage_tempo_ticks (lua_State* L)
{
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    lua_pushnumber (L, msg->getTempoMetaEventTickLength (static_cast<short> (lua_tointeger (L, 2))));
    return 1;
}

midimessage_is (active_sense, isActiveSense)

midimessage_is (start, isMidiStart)
midimessage_is (stop, isMidiStart)
midimessage_is (continue, isMidiStart)
midimessage_is (clock, isMidiClock)

midimessage_is (spp, isSongPositionPointer)
midimessage_get_int (spp_beat, getSongPositionPointerMidiBeat)

midimessage_is (quarter_frame, isQuarterFrame)
midimessage_get_int (quarter_frame_seq, getQuarterFrameSequenceNumber)
midimessage_get_int (quarter_frame_value, getQuarterFrameValue)

midimessage_is (full_frame, isFullFrame)

static int midimessage_full_frame_params (lua_State* L)
{
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    juce::MidiMessage::SmpteTimecodeType tc;
    int h, m, s, f;
    msg->getFullFrameParameters (h, m, s, f, tc);

    lua_pushinteger (L, h);
    lua_pushinteger (L, m);
    lua_pushinteger (L, s);
    lua_pushinteger (L, f);
    lua_pushinteger (L, static_cast<lua_Integer> (tc));

    return 5;
}

midimessage_is (mmc, isMidiMachineControlMessage)
midimessage_get_int (mmc_command, getMidiMachineControlCommand)

static int midimessage_goto (lua_State* L)
{
    auto* msg = *(juce::MidiMessage**) lua_touserdata (L, 1);
    int h, m, s, f;
    auto res = msg->isMidiMachineControlGoto (h, m, s, f);
    lua_pushboolean (L, res);
    lua_pushinteger (L, h);
    lua_pushinteger (L, m);
    lua_pushinteger (L, s);
    lua_pushinteger (L, f);
    return 5;
}

static const luaL_Reg midimessage_methods[] = {
    { "__gc", midimessage_free },

    /// Methods.
    // @section methods

    /// Raw data.
    // @function MidiMessage:data
    { "data", midimessage_data },

    /// Message description.
    // Get human readable information about the message.
    // @function MidiMessage:description
    // @treturn string
    { "description", midimessage_description },

    /// Timestamp.
    // @function MidiMessage:time
    // @treturn number Timestamp
    { "time", midimessage_time },

    /// Change timestamp. The time format depends on the context in which the
    // message is being used.
    // @function MidiMessage:setTime
    // @number t Timestamp to set
    { "setTime", midimessage_set_time },

    /// Add to timestamp.
    // @function MidiMessage:addTime
    // @number dt Delta time to add
    { "addTime", midimessage_add_time },

    /// Create copy with a new timestamp.
    // @function MidiMessage:withTime
    // @number t Timestamp to set
    // @treturn kv.MidiMessage New midi message
    { "withTime", midimessage_with_time },

    /// Midi Channel.
    // @function MidiMessage:channel
    // @treturn int 1-16 if valid channel
    { "channel", midimessage_channel },

    /// Is a channel message.
    // Returns true if the message has the given channel
    // @function MidiMessage:hasChannel
    // @int ch Channel to check
    // @treturn bool
    { "hasChannel", midimessage_isforchannel },

    /// Set the channel.
    // @function MidiMessage:setChannel
    // @int ch Channel to set (1-16)
    { "setChannel", midimessage_set_channel },

    /// Is System Exclusive.
    // @function MidiMessage:isSysex
    // @return True if is SysEx message
    { "isSysex", midimessage_is_sysex },

    /// Sysex Data.
    // @function MidiMessage:sysExData
    // @return Raw sysex bytes
    // @return Sysex data size
    { "sysExData", midimessage_sysExData },

    /// Is note on.
    // @function MidiMessage:isNoteOn
    // @return True if a note on message
    { "isNoteOn", midimessage_is_note_on },

    /// Is note off.
    // @function MidiMessage:isNoteOff
    // @return True if a note on message
    { "isNoteOff", midimessage_is_note_off },

    /// Is note on or off.
    // @function MidiMessage:isNote
    // @return True if a note on message
    { "isNote", midimessage_is_note },

    /// Note number.
    // @function MidiMessage:note
    // @return The note number
    { "note", midimessage_note },

    /// Change note number.
    // @function MidiMessage:setNote
    // @int note New note number
    { "setNote", midimessage_set_note },

    /// Velocity.
    // @function MidiMessage:velocity
    // @treturn int Velocity (0-127)
    { "velocity", midimessage_velocity },

    /// Float velocity.
    // @function MidiMessage:fvelocity
    // @treturn number Velocity (0.0-1.0)
    { "fvelocity", midimessage_velocity_float },

    /// Multiply Velocity.
    // @function MidiMessage:xvelocity
    // @number m Multiplier
    { "xvelocity", midimessage_multiply_velocity },

    /// Change velocity.
    // @function MidiMessage:setVelocity
    // @number v New velocity (0.0-1.0)
    { "setVelocity", midimessage_set_velocity },

    /// Is program change?
    // @function MidiMessage:isProgram
    // @return True if a program change message
    { "isProgram", midimessage_is_program },

    /// Program number.
    // @function MidiMessage:program
    // @treturn int The program number
    { "program", midimessage_program },

    /// Is pitch message.
    // @function MidiMessage:isPitch
    // @return True if a pitch message
    { "isPitch", midimessage_is_pitch },

    /// Pitch.
    // @function MidiMessage:pitch
    // @treturn int Pitch
    { "pitch", midimessage_pitch },

    /// Is after touch message.
    // @function MidiMessage:isAfterTouch
    // @return True if an after touch message
    { "isAfterTouch", midimessage_is_aftertouch },

    /// After touch.
    // @function MidiMessage:afterTouch
    // @treturn int After touch value
    { "afterTouch", midimessage_aftertouch },

    /// Is pressure message.
    // @function MidiMessage:isPressure
    // @return True if an after touch message
    { "isPressure", midimessage_is_pressure },

    /// Pressure.
    // @function MidiMessage:pressure
    // @treturn int Presure value
    { "pressure", midimessage_pressure },

    /// Is controller message.
    // @function MidiMessage:isController
    // @treturn bool True if a controller message
    { "isController", midimessage_is_controller },

    /// Returns the controller number.
    // @function MidiMessage:controller
    // @treturn int The controller number
    { "controller", midimessage_controller },

    /// Returns the controller value.
    // @function MidiMessage:controllerValue
    // @treturn int The controller value
    { "controllerValue", midimessage_controller_value },

    /// Check if this is a particular controller message.
    // @function MidiMessage:isControllerType
    // @int type Controller type to check for
    { "isControllerType", midimessage_is_controller_type },

    /// True if all notes off message.
    // @function MidiMessage:isNotesOff
    // @treturn bool
    { "isNotesOff", midimessage_is_notes_off },

    /// True if all sounds off message.
    // @function MidiMessage:isSoundOff
    // @treturn bool
    { "isSoundOff", midimessage_is_sound_off },

    /// True if reset controllers message.
    // @function MidiMessage:isResetControllers
    // @treturn bool
    { "isResetControllers", midimessage_is_reset_controllers },

    /// True if a Meta message.
    // @function MidiMessage:isMeta
    // @treturn bool
    { "isMeta", midimessage_is_meta },

    /// Returns the metadata type.
    // @function MidiMessage:metaType
    // @treturn int
    { "metaType", midimessage_meta_type },

    /// Returns the metadata.
    // @function MidiMessage:metaData
    { "metaData", midimessage_meta_data },

    /// Returns the metadata length.
    // @function MidiMessage:metaLength
    // @treturn int
    { "metaLength", midimessage_meta_length },

    /// Returns true if a track message.
    // @function MidiMessage:isTrack
    // @treturn bool
    { "isTrack", midimessage_is_track },

    /// Returns true if track end message.
    // @function MidiMessage:isTrackEnd
    // @treturn bool
    { "isTrackEnd", midimessage_is_end_of_track },

    /// Returns true if track name message.
    // @function MidiMessage:isTrackName
    // @treturn bool
    { "isTrackName", midimessage_is_track_name },

    /// Returns true if a text message.
    // @function MidiMessage:isText
    // @treturn bool
    { "isText", midimessage_is_text },

    /// Returns the text.
    // @function MidiMessage:text
    // @treturn string
    { "text", midimessage_text },

    /// Returns true if a tempo message.
    // @function MidiMessage:isTempo
    // @treturn bool
    { "isTempo", midimessage_is_tempo },

    /// Returns the tempo SPQN.
    // @function MidiMessage:tempoSPQN
    // @treturn int
    { "tempoSPQN", midimessage_tempo_seconds_pqn },

    /// Returns the tempo ticks.
    // @function MidiMessage:tempoTicks
    // @treturn int
    { "tempoTicks", midimessage_tempo_ticks },

    /// Returns true if an active sense message.
    // @function MidiMessage:isActiveSense
    // @treturn bool
    { "isActiveSense", midimessage_is_active_sense },

    /// Returns true if a start message.
    // @function MidiMessage:isStart
    // @treturn bool
    { "isStart", midimessage_is_start },

    /// Returns true if a stop message.
    // @function MidiMessage:isStop
    // @treturn bool
    { "isStop", midimessage_is_stop },

    /// Returns true if a continue message.
    // @function MidiMessage:isContinue
    // @treturn bool
    { "isContinue", midimessage_is_continue },

    /// Returns true if is MIDI clock.
    // @function MidiMessage:isClock
    // @treturn bool
    { "isClock", midimessage_is_clock },

    /// Returns true if a song position pointer.
    // @function MidiMessage:isSPP
    // @treturn bool
    { "isSPP", midimessage_is_spp },

    /// Returns true if song position pointer beat.
    // @function MidiMessage:sppBeat
    // @treturn int
    { "sppBeat", midimessage_spp_beat },

    /// Returns true if a quarter frame message.
    // @function MidiMessage:isQuarterFrame
    // @treturn bool
    { "isQuarterFrame", midimessage_is_quarter_frame },

    /// Get the quarter frame sequence.
    // @function MidiMessage:quarterFrameSeq
    // @return The sequence
    { "quarterFrameSeq", midimessage_quarter_frame_seq },

    /// Returns true if song position pointer beat.
    // @function MidiMessage:quarterFrameValue
    // @treturn int The quarter frame value
    { "quarterFrameValue", midimessage_quarter_frame_value },

    /// Returns true if a full frame message.
    // @function MidiMessage:isFullFrame
    // @treturn bool
    { "isFullFrame", midimessage_is_full_frame },

    /// Get full frame parameters.
    // @function MidiMessage:fullFrameParams
    // @return The params
    { "fullFrameParams", midimessage_full_frame_params },

    /// Returns true if a MMC message.
    // @function MidiMessage:isMMC
    // @treturn bool
    { "isMMC", midimessage_is_mmc },

    /// MMC Command.
    // @function MidiMessage:mmcCommand
    { "mmcCommand", midimessage_mmc_command },

    /// MIDI goto.
    // @function MidiMessage:goto
    { "goto", midimessage_goto },

    { nullptr, nullptr }
};

EL_PLUGIN_EXPORT
int luaopen_el_MidiMessage (lua_State* L)
{
    if (luaL_newmetatable (L, EL_MT_MIDI_MESSAGE))
    {
        lua_pushvalue (L, -1); /* duplicate the metatable */
        lua_setfield (L, -2, "__index"); /* mt.__index = mt */
        luaL_setfuncs (L, midimessage_methods, 0);
        lua_pop (L, 1);
    }

    if (luaL_newmetatable (L, EL_MT_MIDI_MESSAGE_TYPE))
    {
        lua_pop (L, 1);
    }

    lua_newtable (L);
    luaL_setmetatable (L, EL_MT_MIDI_MESSAGE_TYPE);
    lua_pushcfunction (L, midimessage_new);
    lua_setfield (L, -2, "new");
    return 1;
}

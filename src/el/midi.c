// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// MIDI factories and utilities.
// Functions in this module **do not** check arguments. It is the responsibility
// of calling code to ensure correct data is passed.
// @author Michael Fisher
// @module el.midi

#include <math.h>

#include "element/element.h"
#include <stddef.h>
#include "el/bytes.h"
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

enum
{
    /* Time */
    SECONDS_PER_MINUTE = 60,
    MINUTES_PER_HOUR = 60,
    SECONDS_PER_HOUR = 3600,
    HOURS_PER_DAY = 24,

    /* Bit manipulation */
    NIBBLE_SHIFT = 4,
    MASK_LOW_NIBBLE = 0x0F,
    MASK_1_BIT = 0x01,
    MASK_2_BITS = 0x03,
    MASK_3_BITS = 0x07,
    MIDI_DATA_BITS = 7,
    MIDI_DATA_MASK = 0x7F,

    /* MIDI status bytes */
    SPP_STATUS = 0xF2,
    MTC_QUARTER_FRAME_STATUS = 0xF1,

    /* SMPTE frame rates */
    SMPTE_24FPS = 24,
    SMPTE_25FPS = 25,
    SMPTE_29_97DF = 29,
    SMPTE_30FPS = 30,

    /* SMPTE rate type encoding */
    SMPTE_TYPE_24 = 0,
    SMPTE_TYPE_25 = 1,
    SMPTE_TYPE_29_97DF = 2,
    SMPTE_TYPE_30 = 3,

    /* MTC Full Frame SysEx */
    MTC_FULL_FRAME_SIZE = 10,
    SYSEX_START = 0xF0,
    SYSEX_END = 0xF7,
    REALTIME_UNIVERSAL_ID = 0x7F,
    ALL_DEVICES_ID = 0x7F,
    MTC_SUB_ID = 0x01,
    FULL_FRAME_SUB_ID = 0x01,
    RATE_TYPE_SHIFT = 5,
    HOURS_MASK = 0x1F
};

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

/// Make a SPP (Song Position Pointer) message.
// @function songpositionpointer
// @int position    Song position in the number of MIDI beats counted from the beginning of the track.
// Each MIDI beat equals 6 MIDI clocks. Since a quarter-note consists of 24 MIDI clocks, that means each
// quarter-note contains 4 MIDI beats.
// @return MIDI message packed as Integer
// @within Messages
static int f_songpositionpointer (lua_State* L)
{
    const lua_Integer position = lua_tointeger (L, 1);
    PackedMessage msg = { .packed = 0x00 };
    msg.data.byte1 = SPP_STATUS;
    msg.data.byte2 = (uint8_t) (position & MIDI_DATA_MASK);
    msg.data.byte3 = (uint8_t) ((position >> MIDI_DATA_BITS) & MIDI_DATA_MASK);
    lua_pushinteger (L, msg.packed);
    return 1;
}

/// Make a quarter frame message.
// @function quarterframe
// @int sequenceNumber The sequence number (0-7) identifying which piece of the timecode is being sent
// @int value The value (0-15) for this piece of timecode
// @return MIDI message packed as Integer
// @within Messages
static int f_quarterframe (lua_State* L)
{
    const lua_Integer sequenceNumber = lua_tointeger (L, 1);
    const lua_Integer value = lua_tointeger (L, 2);
    PackedMessage msg = { .packed = 0x00 };
    msg.data.byte1 = MTC_QUARTER_FRAME_STATUS;
    msg.data.byte2 = (uint8_t) ((sequenceNumber << NIBBLE_SHIFT) | value);
    lua_pushinteger (L, msg.packed);
    return 1;
}

static int check_frames_per_second (lua_State* L, lua_Integer framesPerSecond)
{
    switch (framesPerSecond)
    {
        case SMPTE_24FPS:
        case SMPTE_25FPS:
        case SMPTE_29_97DF:
        case SMPTE_30FPS:
            return 0;
        default:
            return luaL_error (L, "unsupported frame rate: %d (expected 24, 25, 29, or 30)", (int) framesPerSecond);
    }
}

/// Make a quarter frame message from transport state.
// Computes the next MTC quarter frame message based on the current playback
// position. Returns the updated counter, the packed MIDI message, and the
// sample offset within the block. If no message falls within the current
// block, only the unchanged counter is returned.
// Raises an error if framesPerSecond is not 24, 25, 29, or 30.
// @function quarterframetransport
// @int currentSample Current playback position in samples
// @int blockSize Audio block size in samples
// @number sampleRate Audio sample rate in Hz
// @int framesPerSecond MTC frame rate (24, 25, 29, or 30)
// @int sentMessagesCounter Running count of quarter frame messages already sent in the current block
// @return int Updated sentMessagesCounter
// @return[opt] int MIDI message packed as Integer (only when a message falls in this block)
// @return[opt] int Sample offset of the message within the block (only when a message falls in this block)
// @within Messages
static int f_quarterframetransport (lua_State* L)
{
    enum
    {
        QUARTER_FRAMES_PER_FRAME = 4,
        MESSAGES_PER_MTC_CYCLE = 8,

        SEQ_FRAMES_LOW_NIBBLE = 0,
        SEQ_FRAMES_HIGH_NIBBLE = 1,
        SEQ_SECONDS_LOW_NIBBLE = 2,
        SEQ_SECONDS_HIGH_NIBBLE = 3,
        SEQ_MINUTES_LOW_NIBBLE = 4,
        SEQ_MINUTES_HIGH_NIBBLE = 5,
        SEQ_HOURS_LOW_NIBBLE = 6,
        SEQ_HOURS_HIGH_AND_TYPE = 7
    };

    const lua_Integer currentSample = lua_tointeger (L, 1);
    const lua_Integer blockSize = lua_tointeger (L, 2);
    const lua_Number sampleRate = lua_tonumber (L, 3);
    const lua_Integer framesPerSecond = lua_tointeger (L, 4);
    check_frames_per_second (L, framesPerSecond);
    lua_Integer sentMessagesCounter = lua_tointeger (L, 5);

    const lua_Number samplesPerQuarterFrame = sampleRate / (lua_Number) framesPerSecond / QUARTER_FRAMES_PER_FRAME;
    const lua_Integer totalNumberOfQuarterFrames = (lua_Integer) ceil ((lua_Number) currentSample / samplesPerQuarterFrame);
    const lua_Integer totalNumberOfQuarterFrameAtBlockEnd = (lua_Integer) floor ((lua_Number) (currentSample + blockSize) / samplesPerQuarterFrame);

    lua_Integer quarterFrame = totalNumberOfQuarterFrames + sentMessagesCounter;
    if (quarterFrame <= totalNumberOfQuarterFrameAtBlockEnd)
    {
        lua_Integer sequenceNumber = quarterFrame % MESSAGES_PER_MTC_CYCLE;
        const lua_Integer totalNumberOfFramesPassedSinceMessage0 = quarterFrame / MESSAGES_PER_MTC_CYCLE * MESSAGES_PER_MTC_CYCLE / QUARTER_FRAMES_PER_FRAME;

        uint8_t value = 0;

        switch (sequenceNumber)
        {
            case SEQ_FRAMES_LOW_NIBBLE:
                value = (uint8_t) (totalNumberOfFramesPassedSinceMessage0 % framesPerSecond) & MASK_LOW_NIBBLE;
                break;
            case SEQ_FRAMES_HIGH_NIBBLE:
                value = (uint8_t) ((totalNumberOfFramesPassedSinceMessage0 % framesPerSecond) >> NIBBLE_SHIFT) & MASK_1_BIT;
                break;
            case SEQ_SECONDS_LOW_NIBBLE:
                value = (uint8_t) ((totalNumberOfFramesPassedSinceMessage0 / framesPerSecond) % SECONDS_PER_MINUTE) & MASK_LOW_NIBBLE;
                break;
            case SEQ_SECONDS_HIGH_NIBBLE:
                value = (uint8_t) (((totalNumberOfFramesPassedSinceMessage0 / framesPerSecond) % SECONDS_PER_MINUTE) >> NIBBLE_SHIFT) & MASK_2_BITS;
                break;
            case SEQ_MINUTES_LOW_NIBBLE:
                value = (uint8_t) (((totalNumberOfFramesPassedSinceMessage0 / framesPerSecond) / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR) & MASK_LOW_NIBBLE;
                break;
            case SEQ_MINUTES_HIGH_NIBBLE:
                value = (uint8_t) ((((totalNumberOfFramesPassedSinceMessage0 / framesPerSecond) / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR) >> NIBBLE_SHIFT) & MASK_2_BITS;
                break;
            case SEQ_HOURS_LOW_NIBBLE:
                value = (uint8_t) (((totalNumberOfFramesPassedSinceMessage0 / framesPerSecond) / SECONDS_PER_HOUR) % HOURS_PER_DAY) & MASK_LOW_NIBBLE;
                break;
            default: {
                uint8_t smpteType;
                switch (framesPerSecond)
                {
                    case SMPTE_24FPS:
                        smpteType = SMPTE_TYPE_24;
                        break;
                    case SMPTE_25FPS:
                        smpteType = SMPTE_TYPE_25;
                        break;
                    case SMPTE_29_97DF:
                        smpteType = SMPTE_TYPE_29_97DF;
                        break;
                    default:
                        smpteType = SMPTE_TYPE_30;
                        break;
                }
                uint8_t hours = (uint8_t) (((totalNumberOfFramesPassedSinceMessage0 / framesPerSecond) / SECONDS_PER_HOUR) % HOURS_PER_DAY);
                value = ((smpteType << 1) | (hours >> NIBBLE_SHIFT)) & MASK_3_BITS;
                break;
            }
        }

        lua_pushinteger (L, sentMessagesCounter + 1);
        PackedMessage message = { .packed = 0x00 };
        message.data.byte1 = MTC_QUARTER_FRAME_STATUS;
        message.data.byte2 = (uint8_t) ((sequenceNumber << NIBBLE_SHIFT) | value);
        lua_pushinteger (L, message.packed);
        const lua_Integer sampleOffset = (lua_Integer) floor ((lua_Number) quarterFrame * samplesPerQuarterFrame) - currentSample;

        lua_pushinteger (L, sampleOffset);
        return 3;
    }

    lua_pushinteger (L, sentMessagesCounter);
    return 1;
}

/// Make an MTC Full Frame SysEx message.
// Constructs a 10-byte MIDI Time Code Full Frame SysEx message and writes it to the provided buffer.
// The message format is: F0 7F 7F 01 01 hr mn sc fr F7, where the high 3 bits of hr encode the rate type.
// @function mtcfullframe
// @EL_Bytes buffer Buffer to receive the message (must be at least 10 bytes)
// @int hours Hour value (0-23)
// @int minutes Minute value (0-59)
// @int seconds Second value (0-59)
// @int frames Frame value (0-29 depending on rate)
// @int rateType Frame rate type: 0=24fps, 1=25fps, 2=29.97df, 3=30fps
// @return EL_Bytes buffer The buffer passed in (for chaining)
// @return int size Size of the message (always 10)
// @within Messages
static int f_mtcfullframe (lua_State* L)
{
    EL_Bytes* bytes = (EL_Bytes*) lua_touserdata (L, 1);
    if (bytes->size < MTC_FULL_FRAME_SIZE)
    {
        return luaL_error (L, "The buffer is too small: %d", bytes->size);
    }

    lua_Integer hours = lua_tointeger (L, 2);
    lua_Integer minutes = lua_tointeger (L, 3);
    lua_Integer seconds = lua_tointeger (L, 4);
    lua_Integer frames = lua_tointeger (L, 5);
    lua_Integer rateType = lua_tointeger (L, 6);

    bytes->data[0] = SYSEX_START;
    bytes->data[1] = REALTIME_UNIVERSAL_ID;
    bytes->data[2] = ALL_DEVICES_ID;
    bytes->data[3] = MTC_SUB_ID;
    bytes->data[4] = FULL_FRAME_SUB_ID;
    bytes->data[5] = (uint8_t) ((rateType << RATE_TYPE_SHIFT) | (hours & HOURS_MASK));
    bytes->data[6] = (uint8_t) minutes;
    bytes->data[7] = (uint8_t) seconds;
    bytes->data[8] = (uint8_t) frames;
    bytes->data[9] = SYSEX_END;

    lua_pushinteger (L, MTC_FULL_FRAME_SIZE);
    return 1;
}

/// Make an MTC Full Frame SysEx message from transport state.
// Computes the timecode (hours, minutes, seconds, frames) from the current
// playback position and builds the 10-byte Full Frame SysEx message.
// The rate type is derived from framesPerSecond (24→0, 25→1, 29→2, 30→3).
// Raises an error if framesPerSecond is not 24, 25, 29, or 30.
// The message format is: F0 7F 7F 01 01 hr mn sc fr F7, where the high 3 bits
// of hr encode the rate type.
// @function mtcfullframetransport
// @EL_Bytes buffer Buffer to receive the message (must be at least 10 bytes)
// @int currentSample Current playback position in samples
// @number sampleRate Audio sample rate in Hz
// @int framesPerSecond MTC frame rate (24, 25, 29, or 30)
// @return EL_Bytes buffer The buffer passed in (for chaining)
// @return int size of the message (10)
// @within Messages
static int f_mtcfullframetransport (lua_State* L)
{
    EL_Bytes* bytes = (EL_Bytes*) lua_touserdata (L, 1);
    if (bytes->size < MTC_FULL_FRAME_SIZE)
    {
        return luaL_error (L, "The buffer is too small: %d", bytes->size);
    }
    const lua_Integer currentSample = lua_tointeger (L, 2);
    const lua_Number sampleRate = lua_tonumber (L, 3);
    const lua_Integer framesPerSecond = lua_tointeger (L, 4);
    check_frames_per_second (L, framesPerSecond);

    uint8_t rateType;
    switch (framesPerSecond)
    {
        case SMPTE_24FPS:
            rateType = SMPTE_TYPE_24;
            break;
        case SMPTE_25FPS:
            rateType = SMPTE_TYPE_25;
            break;
        case SMPTE_29_97DF:
            rateType = SMPTE_TYPE_29_97DF;
            break;
        default:
            rateType = SMPTE_TYPE_30;
            break;
    }

    const lua_Integer totalFrames = (lua_Integer) floor ((lua_Number) currentSample / sampleRate * (lua_Number) framesPerSecond);
    const lua_Integer totalSeconds = totalFrames / framesPerSecond;
    const uint8_t frames = (uint8_t) (totalFrames % framesPerSecond);
    const uint8_t seconds = (uint8_t) (totalSeconds % SECONDS_PER_MINUTE);
    const uint8_t minutes = (uint8_t) ((totalSeconds / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR);
    const uint8_t hours = (uint8_t) ((totalSeconds / SECONDS_PER_HOUR) % HOURS_PER_DAY);

    bytes->data[0] = SYSEX_START;
    bytes->data[1] = REALTIME_UNIVERSAL_ID;
    bytes->data[2] = ALL_DEVICES_ID;
    bytes->data[3] = MTC_SUB_ID;
    bytes->data[4] = FULL_FRAME_SUB_ID;
    bytes->data[5] = (uint8_t) ((rateType << RATE_TYPE_SHIFT) | (hours & HOURS_MASK));
    bytes->data[6] = minutes;
    bytes->data[7] = seconds;
    bytes->data[8] = frames;
    bytes->data[9] = SYSEX_END;

    lua_pushinteger (L, MTC_FULL_FRAME_SIZE);
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
    { "songpositionpointer", f_songpositionpointer },
    { "quarterframe", f_quarterframe },
    { "quarterframetransport", f_quarterframetransport },
    { "mtcfullframe", f_mtcfullframe },
    { "mtcfullframetransport", f_mtcfullframetransport },

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
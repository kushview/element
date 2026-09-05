--- MIDI Transposer.
--
-- This is a MIDI filter which shifts the note number of all Note On/Off
-- messages by a specified number of semitones. Set the transpose parameter
-- to '0' to bypass the filter. 
--
-- @script      transpose
-- @type        DSP
-- @license     GPL v3
-- @author      Buzz Burrowes

local io            = require ('io')
local midiBuffer    = require ('el.MidiBuffer')
local midi          = require ('el.midi')
local script        = require ('el.script')
local round         = require ('el.round')

local lastSemitones = 0
local lastMidiChannelSeen = 1

-- Buffer to render filtered output
local output        = midiBuffer.new()

local function layout()
    return {
        audio       = { 0, 0 },
        midi        = { 1, 1 },
        control     = {{
            {
                name        = "Transpose",
                symbol      = "transpose",
                min         = -24,
                max         = 24,
                default     = 0
            }
        }}
    }
end

-- prepare for rendering
local function prepare()
    -- reserve 128 bytes of memory and clear the output buffer
    output:reserve (128)
    output:clear()
end

local function process (_, m, p)
    -- Get MIDI input buffer from the MidiPipe
    local input = m:get (1)

    -- Get the transpose amount from the parameter array, and round to integer
    local semitones = round.integer (p[1])

    output:clear()

    -- Send an allNotesOff message is the transposition has changed
    if semitones ~= lastSemitones then
        output:insertPacked (midi.controller (lastMidiChannelSeen, 123, 0), 0)
        lastSemitones = semitones
    end

    -- For each input message, shift the note number if it's a note on/off
    for msg, frame in input:messages() do
        if semitones ~= 0 and (msg:isNoteOn() or msg:isNoteOff()) then
            local note = msg:note(msg) + semitones
            -- clamp to valid MIDI note range
            if note < 0 then note = 0 end
            if note > 127 then note = 127 end
            msg:setNote (note)
            lastMidiChannelSeen = msg:channel()
        end
        output:insert (msg, frame)
     end

    -- DSP scripts use replace processing, so swap in the rendered output
    input:swap (output)
end

return {
    type        = 'DSP',
    layout      = layout,
    parameters  = parameters,
    prepare     = prepare,
    process     = process,
    release     = release,
    dspName     = 'MIDI Transpose'
}

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later

--- MTC Generator.
--
-- Generates MIDI Timecode (MTC) based on the transport state.
--
-- @script      mtc_generator
-- @type        DSP
-- @license     GPL v3
-- @author      Alexandr Zyurkalov

local MidiBuffer = require ('el.MidiBuffer')
local midi       = require ('el.midi')
local bytes      = require ('el.bytes')

-- Buffer to render filtered output
local output = MidiBuffer.new ()

-- Transport state and buffer properties
local wasPlaying = false
local lastSamplePosition = -1
local sampleRate = 44100.0
local bufSize    = 512
local buffer     = bytes.new (10)
local framesPerSecond = 24

local function layout ()
    return {
        audio = { 0, 0 },
        midi = { 0, 1 }
    }
end

-- prepare for rendering
local function prepare (rate, block)
    -- reserve memory and clear the output buffer
    output:reserve (256)
    output:clear ()
    sampleRate = rate
    bufSize    = block
end

local function process (_, m, _, _, t)
    -- Get MIDI input buffer from the MidiPipe
    local input = m:get (1)

    output:clear ()

    local playing        = t:playing ()
    local samplePosition = t:frame ()

    local wasJumped = samplePosition ~= lastSamplePosition and lastSamplePosition >= 0
    if not playing and (wasPlaying or wasJumped) then
        local size = midi.mtcfullframetransport (buffer, samplePosition + bufSize - 1, sampleRate, framesPerSecond)
        output:insertBytes (buffer, size, bufSize)
    end

    if playing then
        local msg, frame, sentMessages = 0

        repeat
            sentMessages, msg, frame = midi.quarterframetransport (samplePosition, bufSize, sampleRate, framesPerSecond, sentMessages)
            output:insertPacked (msg, frame)
        until msg == nil
    end

    wasPlaying = playing
    lastSamplePosition = samplePosition

    -- DSP scripts use replace processing, so swap in the rendered output
    input:swap (output)
end

return {
    type = 'DSP',
    layout = layout,
    prepare = prepare,
    process = process
}

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later
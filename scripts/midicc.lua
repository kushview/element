--- MIDI CC.
--
-- Converts a normalized value (0.0 - 1.0) into a 0-127 MIDI Continuous Controller
-- value. Emits a MIDI CC message on MIDI-out and exposes the scaled value as a
-- control output. A message is only sent when the scaled value changes.
--
-- @script      midicc
-- @type        DSP
-- @license     GPL v3
-- @author      Michael Fisher

local MidiBuffer = require ('el.MidiBuffer')
local midi       = require ('el.midi')
local round      = require ('el.round')

local output    = MidiBuffer.new()
local lastValue = -1

local function layout()
    return {
        audio   = { 0, 0 },
        midi    = { 0, 1 },
        control = { {
            { name = "Value",   symbol = "value",   min = 0.0, max = 1.0, default = 0.0 },
            { name = "CC",      symbol = "cc",      min = 0,   max = 127, default = 1 },
            { name = "Channel", symbol = "channel", min = 1,   max = 16,  default = 1 }
        }, {
            { name = "Value", symbol = "value", min = 0, max = 127, default = 0 }
        } }
    }
end

local function prepare()
    output:reserve (128)
    output:clear()
end

local function process (_, m, p, c)
    local out = m:get (1)
    output:clear()

    local value   = round.integer (p[1] * 127)
    local cc      = round.integer (p[2])
    local channel = round.integer (p[3])

    -- Expose the scaled value as a control output.
    c[1] = value

    -- Only emit a message when the scaled value changes.
    if value ~= lastValue then
        output:insertPacked (midi.controller (channel, cc, value), 0)
        lastValue = value
    end

    out:swap (output)
end

return {
    type    = 'DSP',
    layout  = layout,
    prepare = prepare,
    process = process
}

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later

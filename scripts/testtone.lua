--- Test Tone.
-- A simple sine wave generator, handy as a signal source for checking routing,
-- levels and metering. Frequency is in Hz and Level scales the output amplitude.
--
-- @script      testtone
-- @type        DSP
-- @license     GPL v3
-- @author      Michael Fisher

-- Oscillator phase (radians) and current sample rate, kept across process blocks.
local phase = 0.0
local srate = 44100.0

local function layout()
    return {
        audio   = { 0, 2 },
        midi    = { 0, 0 },
        control = {{
            {
                name    = "Frequency",
                symbol  = "freq",
                label   = "Hz",
                min     = 20.0,
                max     = 20000.0,
                default = 440.0
            },
            {
                name    = "Level",
                symbol  = "level",
                min     = 0.0,
                max     = 1.0,
                default = 0.5
            }
        }}
    }
end

local function prepare (sampleRate, block)
    srate = sampleRate
    phase = 0.0
end

local function process (a, m, p)
    local freq    = p[1]
    local level   = p[2]
    local nframes = a:length()
    local nchans  = a:channels()
    local inc     = (2.0 * math.pi * freq) / srate

    for f = 1, nframes do
        local s = level * math.sin (phase)

        for c = 1, nchans do
            a:set (c, f, s)
        end

        phase = phase + inc
        if phase >= 2.0 * math.pi then
            phase = phase - 2.0 * math.pi
        end
    end
end

return {
    type    = 'DSP',
    layout  = layout,
    prepare = prepare,
    process = process
}

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later

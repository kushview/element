--- Tremolo.
-- Amplitude modulation driven by a sine LFO. Rate sets the modulation speed in
-- Hz and Depth controls how far the level drops at the trough of the wave.
--
-- @script      tremolo
-- @type        DSP
-- @license     GPL v3
-- @author      Michael Fisher

-- LFO phase (radians) and current sample rate, kept across process blocks.
local phase = 0.0
local srate = 44100.0

local function layout()
    return {
        audio   = { 2, 2 },
        midi    = { 0, 0 },
        control = {{
            {
                name    = "Rate",
                symbol  = "rate",
                label   = "Hz",
                min     = 0.1,
                max     = 20.0,
                default = 4.0
            },
            {
                name    = "Depth",
                symbol  = "depth",
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
    local depth   = p[2]
    local nframes = a:length()
    local nchans  = a:channels()
    local inc     = (2.0 * math.pi * freq) / srate

    for f = 1, nframes do
        -- LFO ranges 0..1; gain ranges (1 - depth)..1
        local lfo  = 0.5 + 0.5 * math.sin (phase)
        local gain = 1.0 - depth * (1.0 - lfo)

        for c = 1, nchans do
            a:set (c, f, a:get (c, f) * gain)
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

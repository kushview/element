--- Stereo Amplifier in Lua.
--
-- The code contained implements a simple stereo amplifier plugin. It does not
-- try to smooth the volume parameter and could cause zipper noise. This script 
-- came with Element and is in the public domain.
-- @script  amp
-- @kind    DSP
-- @license GPL v3
-- @author  Michael Fisher

local audio  = require ('kv.audio')
local script = require ('el.script')

--- Gain parameters.
-- Used for fading between changes in volume
local gain1 = 1.0
local gain2 = 1.0

--- Return a table of audio/midi inputs and outputs.
-- This plugin supports stereo in/out with no MIDI
local function amp_layout()
    return {
        audio = { 2, 2 },
        midi  = { 0, 0 }
    }
end

--- Return parameters table.
local function amp_params()
    return {
        {
            name    = "Volume",
            label   = "dB",
            type    = "float",
            flow    = "input",
            min     = -90.0,
            max     = 24.0,
            default = 0.0
        }
    }
end

--- Render audio and midi.
-- Use the provided audio and midi objects to process your plugin
-- @param a     The source kv.AudioBuffer
-- @param m     The source el.MidiPipe
local function amp_process (a, m, params)
    gain2 = audio.togain (params [1])
    a:fade (gain1, gain2)
    gain1 = gain2
end

return script.dsp {
    layout  = amp_layout,
    params  = amp_params,
    process = amp_process
}

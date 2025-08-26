--- Stereo Amplifier in Lua.
--
-- The code contained implements a simple stereo amplifier plugin. It does not
-- try to smooth the volume parameter and could cause zipper noise.
--
-- @script      amp
-- @type        DSP
-- @license     GPL v3
-- @author      Michael Fisher

local audio  = require ('el.audio')

--- Gain parameters.
-- Used for fading between changes in volume
local gain1 = 1.0
local gain2 = 1.0

--- Return a table of audio/midi inputs and outputs.
-- This plugin supports stereo in/out with no MIDI
local function amp_layout()
    return {
        audio   = { 2, 2 },
        midi    = { 0, 0 },
        control = {{
            {
                name        = "Volume",
                symbol      = "volume",
                label       = "dB",
                min         = -90.0,
                max         = 24.0,
                default     = 0.0
            }
        }}
    }
end

--- Render audio and midi.
-- Use the provided audio and midi objects to process your plugin
-- @param a The source el.AudioBuffer
-- @param m The source el.MidiPipe
-- @param p Parameters
-- @param c Controls
-- @param t Time info
local function amp_process (a, m, p, c, t)
    gain2 = audio.togain (p [1])
    a:fade (gain1, gain2)
    gain1 = gain2
end

return {
    type        = 'DSP',
    layout      = amp_layout,
    process     = amp_process
}

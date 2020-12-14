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

local Amp = {}

--- Gain parameters.
-- Used for fading between changes in volume
local gain1 = 1.0
local gain2 = 1.0

--- Initialize the plugin.
function Amp.init()
    gain1 = 1.0
    gain2 = 1.0
end

--- Return a table of audio/midi inputs and outputs.
-- This plugin supports stereo in/out with no MIDI
function Amp.layout()
    return {
        audio = { 2, 2 },
        midi  = { 0, 0 }
    }
end

--- Return parameters table.
function Amp.params()
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

--- Prepare for rendering.
--  Allocate any special data needed here
function Amp.prepare (rate, block)
   -- nothing to do in this example
end

--- Render audio and midi.
-- Use the provided audio and midi objects to process your plugin
-- @param a     The source kv.AudioBuffer
-- @param m     The source el.MidiPipe
function Amp.process (a, m)
   gain2 = audio.togain (Param.values [1])
   a:fade (gain1, gain2)
   gain1 = gain2
end

--- Release node resources.
-- Free any allocated resources in this callback
function Amp.release()
end

--- Save node state
--
-- This is an optional function you can implement to save state.  
-- The host will prepare the IO stream so all you have to do is 
-- `io.write(...)` your data
--
-- Note: Parameter values will automatically be saved and restored,
-- you do not need to handle them here.
function Amp.save()
   io.write ("some custom state data")
end

--- Restore node state
-- This is an optional function you can implement to restore state.
-- The host will prepare the IO stream so all you have to do is 
-- `io.read(...)` your data previsouly written in `node_save()`
function Amp.restore()
   print (io.read ("*a"));
end

return Amp

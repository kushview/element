--- Example - Send MIDI System Exclusive message.
--
-- This example shows how to use a toggle parameter and button to send a SysEx message
-- from a byte array.
--
-- See also - Companion Arduino Sketch:
-- https://gist.github.com/mfisher31/83d84cbb62ec38250e5e9cba4ece7296
--
-- @script      examples.sysex
-- @kind        DSP
-- @license     GPL v3
-- @author      Michael Fisher

local bytes         = require ('kv.bytes')
local script        = require ('el.script')
local MidiBuffer    = require ('kv.MidiBuffer')

local output        = MidiBuffer.new (128)
local sysex         = bytes.new (4)
bytes.set (sysex, 1, 0xF0) -- sysex start
bytes.set (sysex, 2, 0x7D) -- Mfg. ID for educational/development purposes
bytes.set (sysex, 3, 0x00) -- body/value
bytes.set (sysex, 4, 0xF7) -- sysex end

local laststate = bytes.get (sysex, 3)

local function layout()
    return {
        audio = { 0, 0 },
        midi  = { 1, 1 }
    }
end

local function parameters()
    return {
        {
            name        = "Power",
            label       = "power",
            min         = 0,
            max         = 1,
            default     = 0
        }
    }
end

local function process (a, m, p)
    local buf = m:get (1)
    local state = p [1]

    output:clear()
    if state ~= laststate then
        -- update the sysex body
        if state == 0.0 then
            bytes.set (sysex, 3, 0x00)
        else
            bytes.set (sysex, 3, 0x01)
        end
        
        -- add sysex to output buffer
        output:insertbytes (sysex, 4, 1)
        laststate = state
    end

    buf:swap (output)
end

return script.dsp {
    layout      = layout,
    parameters  = parameters,
    process     = process
}

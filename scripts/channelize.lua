--- MIDI Channelizer.
--
-- This is a MIDI filter which forces a specified channel on all messages. Set
-- the channel parameter to '0' to bypass the filter.
--
-- @script      channelize
-- @kind        DSP
-- @license     GPL v3
-- @author      Michael Fisher

local MidiBuffer    = require ('kv.MidiBuffer')
local script        = require ('el.script')
local round         = require ('kv.round')
local output        = MidiBuffer.new()

local function layout()
    return {
        audio = { 0, 0 },
        midi  = { 1, 1 }
    }
end

local function parameters()
    return {
        {
            name        = "Channel",
            label       = "channel",
            min         = 0,
            max         = 16,
            default     = 0
        }
    }
end

local function process (a, m, p)
    local buf = m:get (1)
    local channel = round.integer (p[1])

    a:clear()
    output:clear()
    for msg, frame in buf:messages() do
        if channel > 0 and msg:channel() > 0 then
            msg:setchannel (channel)
        end
        output:addmessage (msg, frame)
    end

    buf:swap (output)
end

return script.dsp {
    layout      = layout,
    parameters  = parameters,
    process     = process
}

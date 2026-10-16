--- MIDI Channelizer.
--
-- This is a MIDI filter which forces a specified channel on all messages. Set
-- the channel parameter to '0' to bypass the filter.
--
-- @script      channelize
-- @type        DSP
-- @license     GPL v3
-- @author      Michael Fisher

local MidiBuffer    = require ('el.MidiBuffer')
local script        = require ('el.script')
local round         = require ('el.round')

-- Buffer to render filtered output
local output        = MidiBuffer.new()

local function layout()
    return {
        audio       = { 0, 0 },
        midi        = { 1, 1 },
        control     = {{
            {
                name        = "Channel",
                symbol      = "channel",
                min         = 0,
                max         = 16,
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
    -- Get MIDI input buffer fromt the MidiPipe
    local input = m:get (1)

    -- Get the channel number from the parameter array, and round to integer
    local channel = round.integer (p[1])

    -- For each input message, set the specified channel
    output:clear()
    for msg, frame in input:messages() do
        if channel > 0 and msg:channel() > 0 then
            msg:setChannel (channel)
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
    release     = release
}

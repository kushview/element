local Amp = {}
local audio  = require ('kv.audio')

local gain1 = 1.0
local gain2 = 1.0
local statedata = "some save data from save/restore"

Amp.rate = 0
Amp.block = 0
Amp.released = false
Amp.initialized = false

function Amp.init()
    Amp.initialized = true
end

function Amp.layout()
    return {
        audio = { 2, 2 },
        midi  = { 0, 0 }
    }
end

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

function Amp.prepare (r, b)
    begintest ("correct rate and block")
    expect (r == 44100)
    expect (b == 4096)
    Amp.rate = r
    Amp.block = b
end

function Amp.process (a, m, params)
    begintest ("params")
    expect (params[1] == -95.0)
    gain2 = audio.togain (params[1])
    a:fade (gain1, gain2)
    gain1 = gain2
end

function Amp.release()
    Amp.released = true
end

function Amp.save()
   io.write (statedata)
end

function Amp.restore()
   begintest ("read saved data")
   expect (io.read ("a") == statedata);
end

return Amp

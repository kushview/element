
table.len = function (t)
    sz = 0
    for _ in pairs(t) do sz = sz + 1 end
    return sz
end

name = "Lua Filter"

ports = {
    {
        index = 0,
        input = true,
        name = "MIDI In",
        slug = "midi_in"
    },
    {
        index = 1,
        input = false,
        name = "MIDI Out",
        slug = "midi_out"
    }
}

function prepare(rate, block)
end

function render (audio, midi)
    -- render the streams
end

function save(memory)
end

function restore(memory)
end

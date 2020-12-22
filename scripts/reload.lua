--- Reload modules included with Element.
-- Applies to modules which have previously been loaded.
-- @script      reload
-- @license     GPL v3
-- @author      Michael Fisher

local mods = {
    ['AudioBuffer']     = 'kv.AudioBuffer',
    ['Bounds']          = 'kv.Bounds',
    ['MidiBuffer']      = 'kv.MidiBuffer',
    ['MidiMessage']     = 'kv.MidiMessage',
    ["Widget"]          = "kv.Widget",
    ["Window"]          = "kv.Window",

    ["audio"]           = "kv.audio",
    ['byte']            = "kv.byte",
    ["midi"]            = "kv.midi",
    ["object"]          = "kv.object",

    ['MidiPipe']        = 'el.MidiPipe',
    ["script"]          = "el.script"
}

local function reload()
    print ("Reload:")
    for name, module in pairs (mods) do
        if package.loaded[module] then
            print ("  * " .. module)
            package.loaded[module] = false
            package.preload[module] = nil
            local m = require (module)
            if _G[name] then _G[name] = m end
            if _ENV[name] then _ENV[name] = _G[name] end
        end
    end
end

reload()
collectgarbage()

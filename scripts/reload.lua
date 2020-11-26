
local mods = {
    ["ui"]      = "element.ui",
    ["widget"]  = "el.Widget",
    ["window"]  = "el.Window",
    ["object"]  = "kv.object"
}

print ("Reloading modules...")
for name, module in pairs (mods) do
    print ("  " .. module)
    _G[name] = nil
    _ENV[name] = nil
    package.loaded[module] = false
    package.preload[module] = nil
    -- require (module)
    -- _ENV[name] = _G[name]
end

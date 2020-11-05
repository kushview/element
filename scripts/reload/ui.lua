
local mods = {
    ["ui"]      = "element.ui",
    ["widget"]  = "element.widget"
}

print ("Reloading modules...")
for name, module in pairs (mods) do
    print ("  " .. module)
    _G[name] = nil
    _ENV[name] = nil
    package.loaded[module] = false
    package.preload[module] = nil
    _G[name] = require (module)
    _ENV[name] = _G[name]
end

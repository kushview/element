--- Console init.
-- Runs when the console is loaded in the GUI. This annonymous script sets global
-- variables, so be careful if you use it directly.
-- @script console

io      = require ('io')
object  = require ('kv.object')
script  = require ('el.script')

--- Built-in console helper table
console = {
    --- Print to stdout instead of the GUI display
    log = function (...)
        local out = ""
        for i = 1, select ('#', ...) do
            out = out .. tostring (select (i, ...)) .. "\t"
        end
        if string.len(out) > 0 then
            io.stdout:write (out .. "\n")
        end
    end
}

--- Console init.
-- Runs when the console is loaded in the GUI. This anonymous script sets global
-- variables, so be careful if you use it directly.
-- @script console
-- @pragma nostrip
-- @type Anonymous

io      = require ('io')
object  = require ('el.object')
command = require ('el.command')
script  = require ('el.script')

console = {
    --- Log to stdout.
    -- Calls `tostring` on each argument then prints the combined result to 
    -- stdout
    -- @function console.log
    -- @param ... Things to log
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

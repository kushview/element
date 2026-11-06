--- On Graph Activated
--
-- This is a script which sends a program change message when the graph is activated.
-- It is useful for setting the initial state of a plugin.
--
-- @script      on_activated
-- @type        DSP
-- @license     GPL v3
-- @author      JVV

local midi = require ('el.midi')

local function layout()
    return {
        audio       = { 0, 0 },
        midi        = { 0, 1 }
    }
end

local function on_graph_activated (a, m, p, c, t)
    m:get (1):insertPacked (midi.program (1, 127), 1)
end

local function process (a, m, p, c, t) end

return {
    type = 'DSP',
    layout = layout,
    on_graph_activated = on_graph_activated,
    process = process
}
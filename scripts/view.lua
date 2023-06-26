--- SysEx MIDI Example.
--
-- @script      el.EmptyView
-- @type        Content
-- @license     GPL v3
-- @author      Michael Fisher

local object = require ('el.object')
local new = object.new

local function instantiate()
    local w = new ('el.View')
    print (tostring (w))
    return w
end

return {
    type = "View",
    instantiate = instantiate,
}

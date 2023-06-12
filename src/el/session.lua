--- Session.
-- @module el.session

local Context = require ('el.Context')
local Session = require ('el.Session')
local Node    = require ('el.Node')
local Graph   = require ('el.Graph')

local M = {}
local session = Context.instance():session()

function M.get()         return session end
function M.toxmlstring() return session:toXmlString() end
function M.name()        return session:name() end

return M

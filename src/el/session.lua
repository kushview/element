--- Session.
-- High level session dealings in Lua.  This module wraps el.Session class
-- in an simplified API.
-- @module el.session

local Context = require ('el.Context')
local Session = require ('el.Session')
local Node    = require ('el.Node')
local Graph   = require ('el.Graph')

local M = {}
local session = Context.instance():session()

function M.toxmlstring() return session:toXmlString() end
function M.name()        return session.name end

return M

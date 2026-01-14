--- Script module.
-- @module el.script
local M = {}

M.DSP        = 'DSP'
M.DSPUI      = 'DSPUI'
M.VIEW       = 'View'
M.GRAPH_VIEW = 'GraphView'
M.ANONYMOUS  = 'Anonymous'

--- Load a known script.
-- @function load
-- @string path The script to run
-- @tparam table env The environment to use or _ENV
-- @treturn table The script's descriptor table
-- @usage script.load ('scriptname')
function M.load (path, env)
    local src, e = package.searchpath (path, package.spath)
    if src then return loadfile (src, 'bt', env or _ENV) end
    return nil, e
end

--- Run a known script.
-- @function exec
-- @string path The script to run
-- @tparam table env The environment to use or _ENV
-- @tparam any ... Arguments passed to script
-- @treturn any Return value from script or no value
-- @usage script.exec ('scriptname')
function M.exec (path, env, ...)
    local invoke, err = M.load (path, env)
    if err then return err end
    return invoke (...)
end

return M

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later

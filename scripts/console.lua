--- Console init.
-- Runs once in the persistent console environment when the console is first
-- opened. It sets globals in that environment, so be careful if you use it
-- directly.
-- @script console
-- @pragma nostrip
-- @type Anonymous

object  = require ('el.object')
command = require ('el.command')
script  = require ('el.script')
Context = require ('el.Context')

--- Returns the active session.
-- A function rather than a value: the session object is replaced when a new
-- session is loaded, so never cache the result.
-- @function session
-- @treturn el.Session
function session()
    return Context.instance():session()
end

console = {
    --- Log to the console.
    -- Calls `tostring` on each argument then prints the combined result.
    -- @function console.log
    -- @param ... Things to log
    log = function (...)
        print (...)
    end
}

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later

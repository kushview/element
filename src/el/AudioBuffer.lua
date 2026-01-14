--- An audio buffer
-- @classmod el.AudioBuffer
-- @pragma nostrip

local AudioBuffer32 = require ('el.AudioBuffer32')
local AudioBuffer64 = require ('el.AudioBuffer64')

local M = {}

function M.new32 (...)
    return AudioBuffer32.new (...)
end

function M.new64 (...)
    return AudioBuffer64.new (...)
end

M.new = M.new32

return M

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later

--- Value.
--
-- A single control output that emits a normalized value (0.0 - 1.0). The value
-- is intended to be driven externally (host automation, MIDI mapping, etc.).
--
-- @script      value
-- @type        DSP
-- @license     GPL v3
-- @author      Michael Fisher

local function layout()
    return {
        audio   = { 0, 0 },
        midi    = { 0, 0 },
        control = { {}, {
            { name = "Value", symbol = "value", min = 0.0, max = 1.0, default = 0.5 }
        } }
    }
end

local function process (_, _, _, _)
    -- Output value is driven externally; nothing to compute here.
end

return {
    type    = 'DSP',
    layout  = layout,
    process = process
}

-- SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
-- SPDX-License-Identifier: GPL-3.0-or-later

--- Spoton Scale Chooser
--
-- Configures chord configurations for Spoton using MIDI CC
--
-- @script      spontonchordchooser
-- @type        DSP
-- @license     GPL v3
-- @author      Lokki

local round   = require('el.round')

-- variables for scales
local scl_idx
local root_note
--current scale in use
local scl_cur = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 }
--all scales that you want to set
local scl_maj = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 } --major
local scl_min = { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0 } --minor
local scl_pma = { 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0 } --major penta
local scl_pmi = { 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0 } --minor penta
local scl_mix = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0 } --mixolydian
local scl_hmi = { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1 } --harmonic minor
local scl_dor = { 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0 } --dorian
local scl_phr = { 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0 } --phrygian
local scl_chr = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 } --chromatic
local scl_lyd = { 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1 } --lydian
local scl_wht = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 } --wholetone
local scl_dip = { 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0 } --diminished halfstep lower
local scl_din = { 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1 } --diminished halfstep higher
local scl_bmi = { 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0 } --bluesscale (minor)
local scl_bma = { 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0 } --blues major
local scl_alt = { 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0 } --altered
local scl_hma = { 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0 } --hm5
local scl_aug = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 } --augemted

--table of all scales for easy recalling
local scl_all = {
    scl_maj, scl_min, scl_pma, scl_pmi, scl_mix, scl_hmi,
    scl_dor, scl_phr, scl_chr, scl_lyd, scl_wht, scl_dip,
    scl_din, scl_bmi, scl_bma, scl_alt, scl_hma, scl_aug
}

local function layout()
    return {
        audio   = { 0, 0 },
        midi    = { 1, 0 },
        control = { {
            {
                name    = "CC_Scale",
                symbol  = "scale",
                min     = 0,
                max     = 127,
                default = 14
            },
            {
                name    = "CC_Root",
                symbol  = "root",
                min     = 0,
                max     = 127,
                default = 6
            },
            {
                name    = "CC_Bypass",
                symbol  = "bypass",
                min     = 0,
                max     = 127,
                default = 90
            }
        }, {
            {
                name   = "C",
                symbol = "c_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "C#",
                symbol = "cis_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "D",
                symbol = "d_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "D#",
                symbol = "dis_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "E",
                symbol = "e_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "F",
                symbol = "f_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "F#",
                symbol = "fis_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "G",
                symbol = "g_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "G#",
                symbol = "gis_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "A",
                symbol = "a_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "A#",
                symbol = "ais_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "B",
                symbol = "b_state",
                min    = 0,
                max    = 1,

            },
            {
                name   = "Bypass",
                symbol = "bypass",
                min    = 0,
                max    = 1,

            },

        }
        }
    }
end

local function process(_, m, p, c)
    -- Get MIDI input buffer fromt the MidiPipe
    local input     = m:get(1)

    -- Get the cc numbers from the parameter array, and round to integer
    local cc_scale  = round.integer(p[1])
    local cc_root   = round.integer(p[2])
    local cc_bypass = round.integer(p[3])
    --process the relevant cc messages

    for msg, frame in input:messages() do
        if msg:isController() then
            if msg:isControllerType(cc_scale) then
                if msg:controllerValue() < #scl_all then
                    scl_idx = msg:controllerValue() + 1
                    scl_cur = scl_all[scl_idx]
                end
            elseif msg:isControllerType(cc_root) then
                root_note = msg:controllerValue()
                for i = 1, 12 do
                    local index = math.tointeger(root_note + i - 1) % 12
                    c[index + 1] = scl_cur[i]
                end
            elseif msg:isControllerType(cc_bypass) then
                if msg:controllerValue() > 0 then
                    c[13] = 0
                else
                    c[13] = 1
                end
            end
        end
    end
end

return {
    type    = 'DSP',
    layout  = layout,
    process = process,
}

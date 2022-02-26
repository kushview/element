--- Slugs.
-- Deal with keyword-like symbols and identifiers
-- @module kv.slug
local M = {}

--- Convert to camel case.
function M.tocamel (str)
	return string.gsub (tostring(str), "_%a+", function (word)
		local first = string.sub (word, 2, 2)
		local rest = string.sub (word, 3)
		return string.upper (first) .. rest
	end)
end

--- Convert to snake case
function M.tosnake (str)
    return string.gsub (tostring (str), "%u", function (c)
        return "_" .. string.lower (c)
    end)
end

--- True if valid slug.
-- For example: allowed a-z, A-Z, 0-9, and _ (no spaces)
function M.valid (str)
	if string.len (string.gsub (string.sub (str, 1, 1), "[%a_]", "")) > 0 then
		return false
	end
	return string.len (string.gsub (tostring (str), "[%a%d_]", "")) == 0
end

return M

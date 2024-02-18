--- A Lua module for basic string manipulation.
-- Provides utilities for trimming, splitting, and converting string cases.
-- @module el.strings

local M = {}

--- Convert a string to camel case.
--
-- @string str The input string to convert
-- @treturn string The input converted to camel case.
function M.tocamel (str)
    return string.gsub (tostring(str), "_%a+", function (word)
        local first = string.sub (word, 2, 2)
        local rest = string.sub (word, 3)
        return string.upper (first) .. rest
    end)
end

--- Convert a string to snake case.
--
-- @string str The input string to convert
-- @treturn string The input converted to snake case.
function M.tosnake (str)
    return string.gsub (tostring (str), "%u", function (c)
        return "_" .. string.lower (c)
    end)
end

--- Checks if a string is a valid slug.
--
-- For example: allowed a-z, A-Z, 0-9, -, and _ (no spaces). This is a relaxed
-- version of @{issymbol} where `-` is allowed.
--
-- @string str The string to check
-- @return True if a valid slug.
function M.isslug (str)
    if string.len (string.gsub (string.sub (str, 1, 1), "[%a_]", "")) > 0 then
        return false
    end
    return string.len (string.gsub (tostring (str), "[%a%d_]", "")) == 0
end

--- Check if a string is a valid symbol.
--
-- The first character of a symbol must be one of _, a-z or A-Z, and subsequent 
-- characters may additionally be 0-9. This is, among other things, a valid C 
-- identifier, and generally compatible in most contexts which have restrictions
-- on string identifiers, such as file paths.
--
-- @string str The string to check
-- @return True is the string is a valid symbol.
function M.issymbol (str)
    if string.len (string.gsub (string.sub (str, 1, 1), "[%a_]", "")) > 0 then
        return false
    end
    return string.len (string.gsub (tostring (str), "[%a%d_]", "")) == 0
end

return M

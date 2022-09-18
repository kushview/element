/// A Parameter.
// A variable property of a Node.
// @classmod el.Parameter
// @pragma nostrip

/// Handlers.
// @section handlers

/// Value changed handler.
// Implement this to handle when the value changes. If you call Parameter:set
// from inside this callback, be careful to not create an infinte callback loop.
// @function Parameter:valuechanged
// @within Handlers
// @usage
// -- The first argument, `self`, will be the Parameter object
// param.valuechanged = function (self)
//     local value = self:get()
//     print (value)
// end

/// Methods.
// @section methods

/// Get the current value.
// @function Parameter:get
// @treturn number The currrent value

/// Set the current value.
// @function Parameter:set
// @number value The new value to set

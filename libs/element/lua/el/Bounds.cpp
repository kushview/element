/// Bounding box
// The value type for this is a 32bit integer and Backed by a juce::Rectangle.
// API is identical to @{kv.Rectangle}.
// @classmod kv.Bounds
// @pragma nostrip

#include "rectangle.hpp"
#define LKV_TYPE_NAME_BOUNDS "Bounds"

using namespace juce;

LKV_EXPORT
int luaopen_el_Bounds (lua_State* L) {
    using B = Rectangle<int>;
    
    auto M = kv::lua::new_rectangle<int> (L, LKV_TYPE_NAME_BOUNDS,
        sol::meta_method::to_string, [](B& self) {
            return kv::lua::to_string (self, LKV_TYPE_NAME_BOUNDS);
        }
    );

    sol::stack::push (L, M);
    return 1;
}

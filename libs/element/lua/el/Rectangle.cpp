/// A rectangle.
// The value type for this is a 32 bit float.
// @classmod kv.Rectangle
// @pragma nostrip

#include "rectangle.hpp"
#define LKV_TYPE_NAME_RECTANGLE "Rectangle"

using namespace juce;

LKV_EXPORT
int luaopen_el_Rectangle (lua_State* L) {
    using R = Rectangle<float>;

    auto M = kv::lua::new_rectangle<float> (L, LKV_TYPE_NAME_RECTANGLE,
        sol::meta_method::to_string, [](R& self) {
            return kv::lua::to_string (self, LKV_TYPE_NAME_RECTANGLE);
        }
    );

    sol::stack::push (L, M);
    return 1;
}

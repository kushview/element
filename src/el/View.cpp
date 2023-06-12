/// Slider widget.
// Is a @{el.Widget}
// @classmod el.View
// @pragma nostrip

#include <element/element.h>
#include <element/ui/view.hpp>
#include "object.hpp"
#include "widget.hpp"

#define LKV_TYPE_NAME_VIEW "View"

namespace element {
namespace lua {

    class View : public element::View
    {
    public:
        View (const sol::table&) {}
        ~View() {}

        static void init (const sol::table& proxy)
        {
            if (auto* impl = object_userdata<lua::View> (proxy))
            {
                impl->proxy = proxy;
                impl->initialize();
            }
        }
        void initialize() {}

    private:
        sol::table proxy;
    };

} // namespace lua
} // namespace element

EL_PLUGIN_EXPORT
int luaopen_el_View (lua_State* L)
{
    // clang-format off
    using namespace juce;
    using element::lua::View;
    namespace lua = element::lua;

    auto T = lua::new_widgettype<View> (
        L, LKV_TYPE_NAME_VIEW, sol::meta_method::to_string, [] (View& self) { 
            return lua::to_string (self, LKV_TYPE_NAME_VIEW); 
        });

    // sol::table T_mt = T[sol::metatable_key];
    // T_mt["__props"].get_or_create<sol::table>().add (
    //     "min", "max", "interval", "style");
    // T_mt["__methods"].get_or_create<sol::table>().add (
    //     "range", "setrange", "value", "setvalue", "settextboxstyle");

    sol::stack::push (L, T);
    return 1;
}

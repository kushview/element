// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/element.h>

#include <element/ui/content.hpp>
#include <element/context.hpp>
#include <element/services.hpp>
#include <element/session.hpp>

#include "object.hpp"
#include "widget.hpp"

#define EL_TYPE_NAME_CONTENT "Content"

using namespace juce;

#if 1

namespace element {
namespace lua {

class Content : public element::Content
{
public:
    using Super = element::Content;

    Content (Context& ctx) : element::Content (ctx) {}
    ~Content() {}

    static void init (const sol::table& proxy)
    {
        if (auto* const impl = object_userdata<Content> (proxy))
            impl->proxy = proxy;
    }

    static auto newUserData (lua_State* L)
    {
        sol::state_view view (L);
        element::Context& ctx = view.globals()["el.context"];
        return std::make_unique<Content> (ctx);
    }

    void presentView (std::unique_ptr<element::View>) override {}
    void presentView (const juce::String&) override {}

    void presentViewObject (const sol::object& o) {}

private:
    Content() = delete;
    sol::table proxy;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Content)
};

} // namespace lua
} // namespace element

// clang-format off
EL_PLUGIN_EXPORT
int luaopen_el_Content (lua_State* L)
{
    namespace lua = element::lua;
    using lua::Content;

    auto T = lua::defineWidget<Content> (L, EL_TYPE_NAME_CONTENT, 
        sol::meta_method::to_string, [] (Content& self) {
            return lua::to_string (self, EL_TYPE_NAME_CONTENT);
        },
        
        "context",          &Content::context,
        "services",         &Content::services,
        "session",          &Content::session,

        "post",             &Content::post,

        "showToolbar",      &Content::setToolbarVisible,
        "refreshToolbar",   &Content::refreshToolbar,

        "showStatusBar",    &Content::setStatusBarVisible,
        "refreshStatusBar", &Content::refreshStatusBar,

        "presentView",      sol::overload (&Content::presentViewObject),

        sol::base_classes, sol::bases<juce::Component, element::Content>());

    lua::Object<Content>::addMethods (T, 
        "content", "services", "post",
        "showToolbar", "refreshToolbar",
        "showStatusBar", "refreshStatusBar",
        "presentView");

    sol::stack::push (L, T);
    return 1;
}
#endif

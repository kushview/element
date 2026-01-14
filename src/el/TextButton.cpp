// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// A Text Button.
// Is a @{el.Widget}.
// @classmod el.TextButton
// @pragma nostrip

#include "object.hpp"
#include "widget.hpp"
#define EL_TYPE_NAME_TEXT_BUTTON "TextButton"

using namespace juce;

namespace element {
namespace lua {

class TextButton : public juce::TextButton,
                   public Button::Listener
{
public:
    TextButton (const sol::table& obj)
    {
        addListener (this);
    }
    ~TextButton()
    {
        removeListener (this);
    }

    static auto newUserData (lua_State* L)
    {
        juce::ignoreUnused (L);
        return std::make_unique<lua::TextButton> (sol::table());
    }

    static void init (const sol::table& proxy)
    {
        if (auto* const impl = object_userdata<TextButton> (proxy))
            impl->widget = proxy;
    }

    /// Handlers.
    // @section handlers

    /// On clicked handler.
    // Executed when the button is clicked by the user.
    // @function TextButton:clicked
    // @tparam el.TextButton self The reference to the clicked button
    void buttonClicked (Button*) override
    {
        try
        {
            if (sol::protected_function f = widget["clicked"])
                f (widget);
        } catch (const sol::error& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

private:
    TextButton() = delete;
    sol::table widget;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextButton)
};

} // namespace lua
} // namespace element

// clang-format off
EL_PLUGIN_EXPORT
int luaopen_el_TextButton (lua_State* L)
{
    namespace lua = element::lua;
    using lua::TextButton;

    auto T = lua::defineWidget<TextButton> (
        L, EL_TYPE_NAME_TEXT_BUTTON, sol::meta_method::to_string, [] (TextButton& self) {
            return lua::to_string (self, EL_TYPE_NAME_TEXT_BUTTON);
        },

        /// Attributes.
        // @section attributes

        /// The button's toggle state (bool)
        // Setting this property **will notify** listeners. If you need to change
        // the toggle state and NOT notify, use the @{setToggleState}
        // method instead.
        // @tfield bool TextButton.togglestate
        "toggleState", sol::property (
            [] (TextButton& self, bool state) { self.setToggleState (state, sendNotificationAsync); }, 
            [] (TextButton& self) { return self.getToggleState(); }
        ),

        /// Displayed text (string)
        // @tfield string TextButton.text
        "text", sol::property ([] (TextButton& self, const char* text) { self.setButtonText (String::fromUTF8 (text)); }, [] (TextButton& self) { return self.getButtonText().toStdString(); }),

        /// Methods.
        // @section methods

        "setToggleState", sol::overload (
            /// Change the button's toggle state.
            // Note this variation **will send** notifications to listeners.
            // @function TextButton:setToggleState
            // @bool state  The new toggle state on or off
            [] (TextButton& self, bool toggled) {
                self.setToggleState (toggled, sendNotification);
            },

            /// Change the button's toggle state.
            // @function TextButton:setToggleState
            // @bool state  The new toggle state on or off
            // @bool notify If true sends notification to listeners
            [] (TextButton& self, bool toggled, bool notify) {
                self.setToggleState (toggled, notify ? sendNotificationSync : dontSendNotification);
            }),

        sol::base_classes,
        sol::bases<Component>());

    auto T_mt = T[sol::metatable_key];
    sol::table __props = T_mt["__props"];

    __props.add (
        "text",
        "toggleState");

    T_mt["__methods"].get_or_create<sol::table>().add (
        "setToggleState");

    sol::stack::push (L, T);
    return 1;
}

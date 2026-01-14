// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Slider widget.
// Is a @{el.Widget}
// @classmod el.Slider
// @pragma nostrip

#include <element/element.h>
#include "object.hpp"
#include "widget.hpp"

#define EL_TYPE_NAME_SLIDER "Slider"

namespace element {
namespace lua {

class Slider : public juce::Slider
{
public:
    Slider (const sol::table&)
        : juce::Slider() {}
    ~Slider() {}

    static void init (const sol::table& proxy)
    {
        if (auto* impl = object_userdata<Slider> (proxy))
        {
            impl->proxy = proxy;
            impl->initialize();
        }
    }

    static auto newUserData (lua_State* L)
    {
        juce::ignoreUnused (L);
        return std::make_unique<lua::Slider> (sol::table());
    }

    void initialize()
    {
        /// Handlers.
        // @section handlers

        /// Value changed.
        //
        // Override this to handle when the value changes.
        // @function Slider:changed
        onValueChange = [this]() {
            if (sol::function f = proxy["changed"])
            {
                auto r = f (proxy);
                if (! r.valid())
                {
                    sol::error e = r;
                    DBG (e.what());
                }
            }
        };

        /// Started to drag.
        //
        // Override this to handle when dragging starts.
        // @function Slider.dragStart
        onDragStart = [this]() {
            if (sol::function f = proxy["dragStart"])
            {
                f (proxy);
            }
        };

        /// Stopped dragging.
        // Override this to handle when dragging ended.
        // @function Slider.dragEnd
        onDragEnd = [this]() {
            if (sol::function f = proxy["dragEnd"])
            {
                f (proxy);
            }
        };
    }

private:
    sol::table proxy;
};

} // namespace lua
} // namespace element

// clang-format off
EL_PLUGIN_EXPORT
int luaopen_el_Slider (lua_State* L)
{
    using namespace juce;
    using element::lua::Slider;
    namespace lua = element::lua;

    auto T = lua::defineWidget<Slider> ( L, EL_TYPE_NAME_SLIDER, 
        sol::meta_method::to_string, [] (Slider& self) { 
            return lua::to_string (self, EL_TYPE_NAME_SLIDER); 
        },

        /// Attributes.
        // @section attributes

        /// Minimum range (readonly).
        // @see Slider.range
        // @tfield number Slider.min
        "min", sol::readonly_property (&Slider::getMinimum),

        /// Maximum range (readonly).
        // @see Slider.range
        // @tfield number Slider.max
        "max", sol::readonly_property (&Slider::getMaximum),

        /// Current step-size (readonly).
        // @see Slider.range
        // @tfield number Slider.interval
        "interval", sol::readonly_property (&Slider::getInterval),

        /// Kind of slider.
        // @tfield int Slider.style
        "style", sol::property (
            [] (Slider& self) -> int { 
                return static_cast<int> (self.getSliderStyle()); 
            }, 
            [] (Slider& self, int style) -> void {
                if (! isPositiveAndBelow (style, Slider::ThreeValueVertical))
                    style = Slider::LinearHorizontal;
                self.setSliderStyle (static_cast<Slider::SliderStyle> (style)); 
            }
        ),

        /// Methods.
        // @section methods

        /// Get the current range.
        // @function Slider:range
        // @return el.Range The current range
        "range", [] (Slider& self) { return self.getRange();  },

        "setRange", sol::overload (
            /// Set the range.
            // @function Slider:setRange
            // @number min
            // @number max
            // @number interval (default: 0.0)
            [] (Slider& self, double min, double max) {
                self.setRange (min, max);
            },
            [] (Slider& self, double min, double max, double interval) {
                self.setRange (min, max, interval);
            },

            /// Set the range.
            // @function Slider:setRange
            // @tparam el.Range range Range to set
            // @number interval Step-size
            [] (Slider& self, Range<double> r, double interval) {
                self.setRange (r, interval);
            }),

        /// Returns the current value.
        // @function Slider:value
        // @treturn number
        "value",
        [] (Slider& self) { return self.getValue(); },

        "setValue", sol::overload (
            /// Change the current value.
            // @function Slider:setValue
            // @number value New value
            // @tparam mixed notify Send notification to listeners
            [] (Slider& self, double value, bool notify) {
                self.setValue (value, notify ? juce::sendNotificationAsync : juce::dontSendNotification);
            },

            [] (Slider& self, double value, int notify) {
                if (! isPositiveAndBelow (notify, 4))
                    notify = sendNotificationAsync;
                self.setValue (value, static_cast<NotificationType> (notify));
            }),

        /// Change TextBox position.
        // @function Slider:setTextBoxStyle
        // @int pos Text box position
        // @bool ro Text box is read only
        // @int width Text box width
        // @int height Text box height
        "setTextBoxStyle",
        [] (Slider& self, lua_Number position, bool readonly, lua_Number width, lua_Number height) {
            const auto pos = static_cast<Slider::TextEntryBoxPosition> (jlimit (0, 4, roundToInt (position)));
            self.setTextBoxStyle (pos, readonly, roundToInt (width), roundToInt (height));
        },

        sol::base_classes,
        sol::bases<juce::Component>());

    /// Styles.
    // @section styles

    /// Linear Horizontal.
    // @tfield int Slider.LinearHorizontal
    T["LinearHorizontal"] = (lua_Integer) Slider::LinearHorizontal;

    /// Linear Vertical.
    // @tfield int Slider.LinearVertical
    T["LinearVertical"] = (lua_Integer) Slider::LinearVertical;

    /// Linear Bar.
    // @tfield int Slider.LinearBar
    T["LinearBar"] = (lua_Integer) Slider::LinearBar;

    /// Linear Bar Vertical.
    // @tfield int Slider.LinearBarVertical
    T["LinearBarVertical"] = (lua_Integer) Slider::LinearBarVertical;

    /// Rotary.
    // @tfield int Slider.Rotary
    T["Rotary"] = (lua_Integer) Slider::Rotary;

    /// Rotary horizontal drag.
    // @tfield int Slider.RotaryHorizontalDrag
    T["RotaryHorizontalDrag"] = (lua_Integer) Slider::RotaryHorizontalDrag;

    /// Rotary vertical drag.
    // @tfield int Slider.RotaryVerticalDrag
    T["RotaryVerticalDrag"] = (lua_Integer) Slider::RotaryVerticalDrag;

    /// Rotary horizontal/vertical drag.
    // @tfield int Slider.RotaryHorizontalVerticalDrag
    T["RotaryHorizontalVerticalDrag"] = (lua_Integer) Slider::RotaryHorizontalVerticalDrag;

    /// Spin buttons.
    // @tfield int Slider.SpinButtons
    T["SpinButtons"] = (lua_Integer) Slider::IncDecButtons;

    /// Two value horizontal.
    // @tfield int Slider.TwoValueHorizontal
    T["TwoValueHorizontal"] = (lua_Integer) Slider::TwoValueHorizontal;

    /// Two value vertical.
    // @tfield int Slider.TwoValueVertical
    T["TwoValueVertical"] = (lua_Integer) Slider::TwoValueVertical;

    /// Three value horizontal.
    // @tfield int Slider.ThreeValueHorizontal
    T["ThreeValueHorizontal"] = (lua_Integer) Slider::ThreeValueHorizontal;

    /// Three value vertical.
    // @tfield int Slider.ThreeValueVertical
    T["ThreeValueVertical"] = (lua_Integer) Slider::ThreeValueVertical;

    /// TextBox Position.
    // @section textbox

    /// No TextBox.
    // @tfield int Slider.TextBoxNone
    T["TextBoxNone"] = (lua_Integer) Slider::NoTextBox;

    /// TextBox on left.
    // @tfield int Slider.TextBoxLeft
    T["TextBoxLeft"] = (lua_Integer) Slider::TextBoxLeft;

    /// TextBox on right.
    // @tfield int Slider.TextBoxRight
    T["TextBoxRight"] = (lua_Integer) Slider::TextBoxRight;

    /// TextBox above.
    // @tfield int Slider.TextBoxAbove
    T["TextBoxAbove"] = (lua_Integer) Slider::TextBoxAbove;

    /// TextBox below.
    // @tfield int Slider.TextBoxBelow
    T["TextBoxBelow"] = (lua_Integer) Slider::TextBoxBelow;

    /// Drag Mode.
    // @section textbox

    /// Not dragging.
    // @tfield int Slider.DragNone
    T["DragNone"] = (lua_Integer) Slider::notDragging;

    /// Absolute dragging.
    // @tfield int Slider.DragAbsolute
    T["DragAbsolute"] = (lua_Integer) Slider::absoluteDrag;

    /// Velocity based dragging.
    // @tfield int Slider.DragVelocity
    T["DragVelocity"] = (lua_Integer) Slider::velocityDrag;

    sol::table T_mt = T[sol::metatable_key];
    T_mt["__props"].get_or_create<sol::table>().add (
        "min", "max", "interval", "style");
    T_mt["__methods"].get_or_create<sol::table>().add (
        "range", "setRange", "value", "setValue", "setTextBoxStyle");

    sol::stack::push (L, T);
    return 1;
}

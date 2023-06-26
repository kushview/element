// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

/// Slider widget.
// Is a @{el.Widget}
// @classmod el.Slider
// @pragma nostrip

#include <element/element.h>
#include "object.hpp"
#include "widget.hpp"

#define LKV_TYPE_NAME_SLIDER "Slider"

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
        // @tfield function Slider.valuechanged
        onValueChange = [this]() {
            if (sol::function f = proxy["valuechanged"])
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
        // @tfield function Slider.dragstart
        onDragStart = [this]() {
            if (sol::function f = proxy["dragstart"])
            {
                f (proxy);
            }
        };

        /// Stopped dragging.
        // @tfield function Slider.dragend
        onDragEnd = [this]() {
            if (sol::function f = proxy["dragend"])
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

    auto T = lua::defineWidget<Slider> ( L, LKV_TYPE_NAME_SLIDER, 
        sol::meta_method::to_string, [] (Slider& self) { 
            return lua::to_string (self, LKV_TYPE_NAME_SLIDER); 
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
        "style", sol::property ([] (Slider& self) -> int { return static_cast<int> (self.getSliderStyle()); }, [] (Slider& self, int style) -> void {
                if (! isPositiveAndBelow (style, Slider::ThreeValueVertical))
                    style = Slider::LinearHorizontal;
                self.setSliderStyle (static_cast<Slider::SliderStyle> (style)); }),

        /// Methods.
        // @section methods

        /// Get the current range.
        // @function Slider:range
        // @return kv.Range The current range
        "range",
        [] (Slider& self) {
            return self.getRange();
        },

        "setrange",
        sol::overload (
            /// Set the range.
            // @function Slider:setrange
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
            // @function Slider:setrange
            // @tparam kv.Range range Range to set
            // @number interval Step-size
            [] (Slider& self, Range<double> r, double interval) {
                self.setRange (r, interval);
            }),

        /// Returns the current value.
        // @function Slider:value
        // @treturn number
        "value",
        [] (Slider& self) { return self.getValue(); },

        "setvalue",
        sol::overload (
            /// Change the current value.
            // @function Slider:setvalue
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
        // @function Slider:textboxstyle
        // @int pos Text box position
        // @bool ro Text box is read only
        // @int width Text box width
        // @int height Text box height
        "settextboxstyle",
        [] (Slider& self, lua_Number position, bool readonly, lua_Number width, lua_Number height) {
            const auto pos = static_cast<Slider::TextEntryBoxPosition> (jlimit (0, 4, roundToInt (position)));
            self.setTextBoxStyle (pos, readonly, roundToInt (width), roundToInt (height));
        },

        sol::base_classes,
        sol::bases<juce::Component>());

    /// Styles.
    // @section styles

    /// Linear Horizontal.
    // @tfield int Slider.LINEAR_HORIZONTAL
    T["LINEAR_HORIZONTAL"] = (lua_Integer) Slider::LinearHorizontal;

    /// Linear Vertical.
    // @tfield int Slider.LINEAR_VERTICAL
    T["LINEAR_VERTICAL"] = (lua_Integer) Slider::LinearVertical;

    /// Linear Bar.
    // @tfield int Slider.LINEAR_BAR
    T["LINEAR_BAR"] = (lua_Integer) Slider::LinearBar;

    /// Linear Bar Vertical.
    // @tfield int Slider.LINEAR_BAR_VERTICAL
    T["LINEAR_BAR_VERTICAL"] = (lua_Integer) Slider::LinearBarVertical;

    /// Rotary.
    // @tfield int Slider.ROTARY
    T["ROTARY"] = (lua_Integer) Slider::Rotary;

    /// Rotary horizontal drag.
    // @tfield int Slider.ROTARY_HORIZONTAL_DRAG
    T["ROTARY_HORIZONTAL_DRAG"] = (lua_Integer) Slider::RotaryHorizontalDrag;

    /// Rotary vertical drag.
    // @tfield int Slider.ROTARY_VERTICAL_DRAG
    T["ROTARY_VERTICAL_DRAG"] = (lua_Integer) Slider::RotaryVerticalDrag;

    /// Rotary horizontal/vertical drag.
    // @tfield int Slider.ROTARY_HORIZONTAL_VERTICAL_DRAG
    T["ROTARY_HORIZONTAL_VERTICAL_DRAG"] = (lua_Integer) Slider::RotaryHorizontalVerticalDrag;

    /// Spin buttons.
    // @tfield int Slider.SPIN_BUTTONS
    T["SPIN_BUTTONS"] = (lua_Integer) Slider::IncDecButtons;

    /// Two value horizontal.
    // @tfield int Slider.TWO_VALUE_HORIZONTAL
    T["TWO_VALUE_HORIZONTAL"] = (lua_Integer) Slider::TwoValueHorizontal;

    /// Two value vertical.
    // @tfield int Slider.TWO_VALUE_VERTICAL
    T["TWO_VALUE_VERTICAL"] = (lua_Integer) Slider::TwoValueVertical;

    /// Three value horizontal.
    // @tfield int Slider.THREE_VALUE_HORIZONTAL
    T["THREE_VALUE_HORIZONTAL"] = (lua_Integer) Slider::ThreeValueHorizontal;

    /// Three value vertical.
    // @tfield int Slider.THREE_VALUE_VERTICAL
    T["THREE_VALUE_VERTICAL"] = (lua_Integer) Slider::ThreeValueVertical;

    /// TextBox Position.
    // @section textbox

    /// No TextBox.
    // @tfield int Slider.TEXT_BOX_NONE
    T["TEXT_BOX_NONE"] = (lua_Integer) Slider::NoTextBox;

    /// TextBox on left.
    // @tfield int Slider.TEXT_BOX_LEFT
    T["TEXT_BOX_LEFT"] = (lua_Integer) Slider::TextBoxLeft;

    /// TextBox on right.
    // @tfield int Slider.TEXT_BOX_RIGHT
    T["TEXT_BOX_RIGHT"] = (lua_Integer) Slider::TextBoxRight;

    /// TextBox above.
    // @tfield int Slider.TEXT_BOX_ABOVE
    T["TEXT_BOX_ABOVE"] = (lua_Integer) Slider::TextBoxAbove;

    /// TextBox below.
    // @tfield int Slider.TEXT_BOX_BELOW
    T["TEXT_BOX_BELOW"] = (lua_Integer) Slider::TextBoxBelow;

    /// Drag Mode.
    // @section textbox

    /// Not dragging.
    // @tfield int Slider.DRAG_NONE
    T["DRAG_NONE"] = (lua_Integer) Slider::notDragging;

    /// Absolute dragging.
    // @tfield int Slider.DRAG_ABSOLUTE
    T["DRAG_ABSOLUTE"] = (lua_Integer) Slider::absoluteDrag;

    /// Velocity based dragging.
    // @tfield int Slider.DRAG_VELOCITY
    T["DRAG_VELOCITY"] = (lua_Integer) Slider::velocityDrag;

    sol::table T_mt = T[sol::metatable_key];
    T_mt["__props"].get_or_create<sol::table>().add (
        "min", "max", "interval", "style");
    T_mt["__methods"].get_or_create<sol::table>().add (
        "range", "setrange", "value", "setvalue", "settextboxstyle");

    sol::stack::push (L, T);
    return 1;
}

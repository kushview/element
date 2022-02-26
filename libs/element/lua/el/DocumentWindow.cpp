/// A Document Window.
// A window with a title bar and optional buttons. Is a @{kv.Widget}
// Backed by a JUCE DocumentWindow
// @classmod kv.DocumentWindow
// @pragma nostrip

#include "object.hpp"
#include "widget.hpp"
#include "lua-kv.hpp"
#include LKV_JUCE_HEADER

#define LKV_TYPE_NAME_WINDOW     "DocumentWindow"

using namespace juce;

namespace kv {
namespace lua {

class DocumentWindow : public juce::DocumentWindow
{
public:
    DocumentWindow (const sol::table&)
        : juce::DocumentWindow ("", Colours::black, DocumentWindow::allButtons, true)
    { }

    ~DocumentWindow() override
    {
        widget = sol::lua_nil;
    }

    static void init (const sol::table& proxy) {
        if (auto* const impl = object_userdata<DocumentWindow> (proxy)) {
            impl->widget = proxy;
            impl->setUsingNativeTitleBar (true);
            impl->setResizable (true, false);
        }
    }

    void resized() override
    {
        juce::DocumentWindow::resized();
    }

    /// Close button pressed.
    // Called when the title bar close button is pressed.
    // @function DocumentWindow:closepressed
    // @within Handlers
    void closeButtonPressed() override
    {
        if (sol::safe_function f = widget ["closepressed"])
            f (widget);
    }

    void setContent (const sol::object& child)
    {
        switch (child.get_type())
        {
            case sol::type::table:
            {
                if (Component* const comp = object_userdata<Component> (child))
                {
                    content = child;
                    setContentNonOwned (comp, true);
                }
                else
                {
                    // DBG("failed to set widget");
                }
                break;
            }
            
            case sol::type::lua_nil:
            {
                clearContentComponent();
                content = sol::lua_nil;
                break;
            }

            default:
                break;
        }
    }

    sol::table getContent() const { return content; }

private:
    sol::table widget;
    sol::table content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DocumentWindow)
};

}}

LKV_EXPORT
int luaopen_el_DocumentWindow (lua_State* L) {
    using kv::lua::DocumentWindow;

    auto T = kv::lua::new_widgettype<DocumentWindow> (L, LKV_TYPE_NAME_WINDOW,
        sol::meta_method::to_string, [](DocumentWindow& self) {
            return kv::lua::to_string (self, LKV_TYPE_NAME_WINDOW);
        },

        /// Attributes.
        // @section attributes

        /// Methods.
        // @section methods

        /// Add to desktop.
        // @function DocumentWindow:addtodesktop
        "addtodesktop",   [](DocumentWindow& self) { self.addToDesktop(); },

        /// Change the viewed content.
        // @function DocumentWindow:setcontent
        // @tparam kv.Widget widget Content to set
        "setcontent",      &DocumentWindow::setContent,

        /// Returns the viewed content widget.
        // @function DocumentWindow:content
        // @treturn kv.Widget
        "content",         &DocumentWindow::getContent,
        
        sol::base_classes, sol::bases<juce::DocumentWindow,
                                      juce::ResizableWindow,
                                      juce::TopLevelWindow,
                                      juce::Component,
                                      juce::MouseListener>()
    );

    auto T_mt = T[sol::metatable_key];

    sol::table props = T_mt["__props"];
    props.add ();
    
    sol::table methods = T_mt["__methods"];
    methods.add (
        "content", "setcontent", "addtodesktop"
    );

    sol::stack::push (L, T);
    return 1;
}

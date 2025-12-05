// Copyright: Copied from JUCE framework.
// SPDX-License-Identifier: GPL3-or-later

#define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1
#define JUCE_CORE_INCLUDE_OBJC_HELPERS 1
// #define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
// #define JUCE_GUI_BASICS_INCLUDE_SCOPED_THREAD_DPI_AWARENESS_SETTER 1
// #define JUCE_GRAPHICS_INCLUDE_COREGRAPHICS_HELPERS 1

// clang-format off
#include <element/juce/audio_processors.hpp>
#include "nsviewwithparent.hpp"
// clang-format on

using namespace juce;

namespace element
{

struct NSViewComponentWithParent::InnerNSView : public juce::ObjCClass<NSView>
{
    InnerNSView()
        : ObjCClass("JuceInnerNSView_")
    {
        addIvar<NSViewComponentWithParent*>("owner");

        addMethod(@selector(isOpaque), isOpaque);
        addMethod(@selector(didAddSubview:), didAddSubview);

        registerClass();
    }

    static BOOL isOpaque(id, SEL) { return YES; }

    static void nudge(id self)
    {
        if (auto* owner = getIvar<NSViewComponentWithParent*>(self, "owner"))
            if (owner->wantsNudge == WantsNudge::yes)
                owner->triggerAsyncUpdate();
    }

    static void didAddSubview(id self, SEL, NSView*) { nudge(self); }
};

NSViewComponentWithParent::NSViewComponentWithParent(WantsNudge shouldNudge)
    : wantsNudge(shouldNudge)
{
    auto* view = [[getViewClass().createInstance() init] autorelease];
    object_setInstanceVariable(view, "owner", this);
    setView(view);
}

NSViewComponentWithParent::NSViewComponentWithParent(AudioPluginInstance& instance)
    : NSViewComponentWithParent(getWantsNudge(instance)) {}

NSViewComponentWithParent::~NSViewComponentWithParent()
{
    if (auto* view = static_cast<NSView*>(getView()))
        object_setInstanceVariable(view, "owner", nullptr);

    cancelPendingUpdate();
}

NSViewComponentWithParent::WantsNudge NSViewComponentWithParent::getWantsNudge(AudioPluginInstance& instance)
{
    PluginDescription pd;
    instance.fillInPluginDescription(pd);
    return pd.manufacturerName == "FabFilter" ? WantsNudge::yes : WantsNudge::no;
}

void NSViewComponentWithParent::handleAsyncUpdate()
{
    if (auto* peer = getTopLevelComponent()->getPeer())
    {
        auto* view = static_cast<NSView*>(getView());
        const auto newArea = peer->getAreaCoveredBy(*this);
        [view setFrame:makeCGRect(newArea.withHeight(newArea.getHeight() + 1))];
        [view setFrame:makeCGRect(newArea)];
    }
}

NSViewComponentWithParent::InnerNSView& NSViewComponentWithParent::getViewClass()
{
    static InnerNSView result;
    return result;
}

} // namespace element

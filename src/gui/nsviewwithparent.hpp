// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/gui_extra.hpp>

namespace juce {
class AudioPluginInstance;
}

namespace element {
#if JUCE_MAC
//==============================================================================
/*  This is an NSViewComponent which holds a long-lived NSView which acts
        as the parent view for plugin editors.

        Note that this component does not auto-resize depending on the bounds
        of the owned view. VST2 and VST3 plugins have dedicated interfaces to
        request that the editor bounds are updated. We can call `setSize` on this
        component from inside those dedicated callbacks.
    */
struct NSViewComponentWithParent : public juce::NSViewComponent,
                                   private juce::AsyncUpdater
{
    enum class WantsNudge
    {
        no,
        yes
    };

    explicit NSViewComponentWithParent (WantsNudge shouldNudge);
    explicit NSViewComponentWithParent (juce::AudioPluginInstance& instance);
    ~NSViewComponentWithParent() override;

    JUCE_DECLARE_NON_COPYABLE (NSViewComponentWithParent)
    JUCE_DECLARE_NON_MOVEABLE (NSViewComponentWithParent)

private:
    WantsNudge wantsNudge = WantsNudge::no;
    static WantsNudge getWantsNudge (juce::AudioPluginInstance& instance);
    void handleAsyncUpdate() override;

    struct InnerNSView;
    static InnerNSView& getViewClass();
};

#endif

} // namespace element

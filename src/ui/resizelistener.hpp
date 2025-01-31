
#pragma once

#include <element/juce/gui_basics.hpp>

namespace element {
struct PhysicalResizeListener
{
    virtual ~PhysicalResizeListener() = default;
    virtual void viewRequestedResizeInPhysicalPixels (int width, int height) = 0;
};

struct LogicalResizeListener
{
    virtual ~LogicalResizeListener() = default;
    virtual void viewRequestedResizeInLogicalPixels (int width, int height) = 0;
};

struct ViewSizeListener : private juce::ComponentMovementWatcher
{
    ViewSizeListener (Component& c, PhysicalResizeListener& l)
        : ComponentMovementWatcher (&c), listener (l)
    {
    }

    void componentMovedOrResized (bool, bool wasResized) override
    {
        if (wasResized)
        {
            const auto physicalSize = Desktop::getInstance().getDisplays().logicalToPhysical (getComponent()->localAreaToGlobal (getComponent()->getLocalBounds()));
            const auto width = physicalSize.getWidth();
            const auto height = physicalSize.getHeight();

            if (width > 10 && height > 10)
                listener.viewRequestedResizeInPhysicalPixels (width, height);
        }
    }

    void componentPeerChanged() override {}
    void componentVisibilityChanged() override {}

    using ComponentMovementWatcher::componentMovedOrResized;
    using ComponentMovementWatcher::componentVisibilityChanged;

    PhysicalResizeListener& listener;
};
} // namespace element

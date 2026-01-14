// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "engine/graphnode.hpp"
#include "engine/ionode.hpp"
#include <element/devices.hpp>

namespace element {

class RootGraph : public GraphNode
{
public:
    RootGraph() = delete;
    RootGraph (Context&);
    ~RootGraph() {}

    enum RenderMode
    {
        SingleGraph = 0,
        Parallel = (1 << 0)
    };

    inline static bool renderModeValid (const int mode)
    {
        return mode == SingleGraph || mode == Parallel;
    }

    inline static String getSlugForRenderMode (const RenderMode mode)
    {
        switch (mode)
        {
            case SingleGraph:
                return "single";
                break;
            case Parallel:
                return "parallel";
                break;
        }
        return String();
    }

    void refreshPorts() override;

    void setPlayConfigFor (AudioIODevice* device);
    void setPlayConfigFor (const DeviceManager::AudioDeviceSetup& setup);
    void setPlayConfigFor (DeviceManager&);

    inline constexpr RenderMode getRenderMode() const noexcept { return renderMode; }
    inline String getRenderModeSlug() const noexcept { return getSlugForRenderMode (renderMode); }
    inline constexpr bool isSingle() const noexcept { return getRenderMode() == SingleGraph; }

    inline void setRenderMode (const RenderMode mode)
    {
        if (renderMode == mode)
            return;
        ScopedLock sl (getPropertyLock());
        renderMode = mode;
    }

    inline void setMidiProgram (const int program)
    {
        if (program == midiProgram)
            return;
        ScopedLock sl (getPropertyLock());
        midiProgram = program;
    }

    /** Returns the index used for rendering in the audio engine.

        If the return value is less than 0, it means the graph is not attached.
     */
    inline constexpr int getEngineIndex() const noexcept { return engineIndex; }

private:
    friend class AudioEngine;
    friend struct RootGraphRender;
    using IODeviceType = IONode::IODeviceType;
    ProcessorPtr ioNodes[IONode::numDeviceTypes];
    int midiChannel = 0;
    int midiProgram = -1;
    int engineIndex = -1;
    RenderMode renderMode = Parallel;
};

} // namespace element

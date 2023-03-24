
#pragma once

#include "engine/graphnode.hpp"
#include "engine/ionode.hpp"
#include <element/devicemanager.hpp>

namespace element {

class RootGraph : public GraphNode
{
public:
    RootGraph();
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

    void setValueTree (const ValueTree& tree);
    void setPlayConfigFor (AudioIODevice* device);
    void setPlayConfigFor (const DeviceManager::AudioDeviceSetup& setup);
    void setPlayConfigFor (DeviceManager&);

    inline RenderMode getRenderMode() const { return renderMode; }
    inline String getRenderModeSlug() const { return getSlugForRenderMode (renderMode); }
    inline bool isSingle() const { return getRenderMode() == SingleGraph; }

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

    const String getInputChannelName (int channelIndex) const;
    const String getOutputChannelName (int channelIndex) const;
    const String getAudioInputDeviceName() const { return graphName; }
    const String getAudioOutputDeviceName() const { return graphName; }

    /** Returns the index in the audio engine.

        If the return value is less than 0, it means the graph is not attached.
     */
    int getEngineIndex() const { return engineIndex; }

private:
    friend class AudioEngine;
    friend struct RootGraphRender;
    using IODeviceType = IONode::IODeviceType;
    NodeObjectPtr ioNodes[IONode::numDeviceTypes];
    String graphName = "Device";
    StringArray audioInputNames;
    StringArray audioOutputNames;
    int midiChannel = 0;
    int midiProgram = -1;
    int engineIndex = -1;
    RenderMode renderMode = Parallel;

    void updateChannelNames (AudioIODevice* device);
};
} // namespace element

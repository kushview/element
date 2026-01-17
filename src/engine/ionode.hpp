// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/core.hpp>
#include <element/processor.hpp>

namespace element {

class GraphNode;

//==========================================================================
/** A special type of Processor that can live inside an ProcessorGraph
    in order to use the audio that comes into and out of the graph itself.

    If you create an IONode in "input" mode, it will act as a
    node in the graph which delivers the audio that is coming into the parent
    graph. This allows you to stream the data to other nodes and process the
    incoming audio.

    Likewise, one of these in "output" mode can be sent data which it will add to
    the sum of data being sent to the graph's output.

    @see GraphNode
*/
class IONode : public Processor
{
public:
    /** Specifies the mode in which this processor will operate.
    */
    enum IODeviceType
    {
        audioInputNode = 0, /**< In this mode, the processor has output channels
                                    representing all the audio input channels that are
                                    coming into its parent audio graph. */
        audioOutputNode, /**< In this mode, the processor has input channels
                                    representing all the audio output channels that are
                                    going out of its parent audio graph. */
        midiInputNode, /**< In this mode, the processor has a midi output which
                                    delivers the same midi data that is arriving at its
                                    parent graph. */
        midiOutputNode, /**< In this mode, the processor has a midi input and
                                    any data sent to it will be passed out of the parent
                                    graph. */
        numDeviceTypes
    };

    //==============================================================================
    IONode (const IODeviceType type);
    ~IONode();

    //==============================================================================
    /** Returns the mode of this processor. */
    IODeviceType getType() const { return type; }
    PortType getPortType() const noexcept;

    /** Returns the parent graph to which this processor belongs, or nullptr if it
        hasn't yet been added to one. */
    GraphNode* getParentGraph() const { return graph; }

    /** True if this is an audio or midi input. */
    bool isInput() const;
    /** True if this is an audio or midi output. */
    bool isOutput() const;

    void fillInPluginDescription (juce::PluginDescription& d) const;

    const juce::String getInputChannelName (int channelIndex) const;
    const juce::String getOutputChannelName (int channelIndex) const;

    //==========================================================================
    void refreshPorts() override;
    bool wantsContext() const noexcept override { return true; }
    void prepareToRender (double, int) override;
    void releaseResources() override;
    void render (RenderContext&) override;
    void getState (juce::MemoryBlock&) override {}
    void setState (const void*, int sizeInBytes) override {}

    //==========================================================================
    /** @internal */
    void setParentGraph (GraphNode*);

private:
    const IODeviceType type;
    GraphNode* graph;
    void updateName();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IONode)
};

} // namespace element

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "engine/ionode.hpp"
#include "engine/graphnode.hpp"
#include "nodes/nodetypes.hpp"

namespace element {

IONode::IONode (const IODeviceType type_)
    : Processor (0),
      type (type_),
      graph (nullptr)
{
    updateName();
}

IONode::~IONode()
{
}

void IONode::updateName()
{
    String name = getPortType().getName();
    name << " " << String (isInput() ? "In" : "Out");
    setName (name);
}

PortType IONode::getPortType() const noexcept
{
    switch (getType())
    {
        case audioInputNode:
        case audioOutputNode:
            return PortType::Audio;
            break;
        case midiInputNode:
        case midiOutputNode:
            return PortType::Midi;
            break;
        default:
            break;
    }
    return PortType::Unknown;
}

void IONode::refreshPorts()
{
    PortList list;
    if (auto* const graph = getParentGraph())
    {
        const auto portType = getPortType();
        int index = 0, channel = 0;

        for (int i = 0; i < graph->getNumPorts (getPortType(), isInput()); ++i)
        {
            String symbol = portType.getSlug(),
                   name = portType.getName();

            symbol << "_" << String (isInput() ? "in" : "out") << "_" << String (channel + 1);
            name << " " << String (isInput() ? "In" : "Out") << " " << String (channel + 1);
            list.add (portType, index++, channel++, symbol, name, ! isInput());
        }
    }

    setPorts (list);
}

void IONode::fillInPluginDescription (PluginDescription& d) const
{
    d.name = getName();
    d.uniqueId = d.name.hashCode();
    d.category = "I/O Devices";
    d.pluginFormatName = "Internal";
    d.manufacturerName = EL_NODE_FORMAT_AUTHOR;
    d.version = "1.0";
    d.isInstrument = false;

    switch (static_cast<int> (this->type))
    {
        case audioInputNode:
            d.fileOrIdentifier = "audio.input";
            break;
        case audioOutputNode:
            d.fileOrIdentifier = "audio.output";
            break;
        case midiInputNode:
            d.fileOrIdentifier = "midi.input";
            break;
        case midiOutputNode:
            d.fileOrIdentifier = "midi.output";
            break;
    }
}

void IONode::prepareToRender (double r, int b)
{
    setRenderDetails (r, b);
    jassert (graph != nullptr);
}

void IONode::releaseResources() {}

void IONode::render (RenderContext& rc) // AudioSampleBuffer& buffer, MidiPipe& midiPipe, AudioSampleBuffer&)
{
    jassert (graph != nullptr);
    // jassert (midiPipe.getNumBuffers() > 0);
    auto& midiMessages = *rc.midi.getWriteBuffer (0);
    switch (type)
    {
        case audioOutputNode: {
            for (int i = jmin (graph->currentAudioOutputBuffer.getNumChannels(),
                               rc.audio.getNumChannels());
                 --i >= 0;)
            {
                graph->currentAudioOutputBuffer.addFrom (i, 0, rc.audio, i, 0, rc.audio.getNumSamples());
            }

            break;
        }

        case audioInputNode: {
            for (int i = jmin (graph->currentAudioInputBuffer->getNumChannels(),
                               rc.audio.getNumChannels());
                 --i >= 0;)
            {
                rc.audio.copyFrom (i, 0, *graph->currentAudioInputBuffer, i, 0, rc.audio.getNumSamples());
            }

            break;
        }

        case midiOutputNode:
            graph->currentMidiOutputBuffer.clear();
            graph->currentMidiOutputBuffer.addEvents (midiMessages, 0, rc.audio.getNumSamples(), 0);
            midiMessages.clear();
            break;

        case midiInputNode:
            midiMessages.clear();
            midiMessages.addEvents (*graph->currentMidiInputBuffer, 0, rc.audio.getNumSamples(), 0);
            graph->currentMidiInputBuffer->clear();
            break;

        default:
            break;
    }
}

const String IONode::getInputChannelName (int channelIndex) const
{
    switch (type)
    {
        case audioOutputNode:
            return "Output " + String (channelIndex + 1);
        case midiOutputNode:
            return "Midi Output";
        default:
            break;
    }

    return String();
}

const String IONode::getOutputChannelName (int channelIndex) const
{
    switch (type)
    {
        case audioInputNode:
            return "Input " + String (channelIndex + 1);
        case midiInputNode:
            return "Midi Input";
        default:
            break;
    }

    return String();
}

bool IONode::isInput() const { return type == audioInputNode || type == midiInputNode; }
bool IONode::isOutput() const { return type == audioOutputNode || type == midiOutputNode; }

void IONode::setParentGraph (GraphNode* const newGraph)
{
    graph = newGraph;

    // Ensure the parent graph has a minimum port count for this IONode's type.
    // Default: 2 channels for audio (stereo), 1 channel for MIDI.
    if (graph != nullptr)
    {
        const auto portType = getPortType();
        const int currentCount = graph->getNumPorts (portType, isInput());
        if (currentCount == 0)
        {
            const int defaultCount = (portType == PortType::Audio) ? 2 : 1;
            graph->setNumPorts (portType, defaultCount, isInput(), false);
        }
    }

    refreshPorts();
}

} // namespace element

/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <element/nodefactory.hpp>

#include "engine/nodes/BaseProcessor.h"
#include "engine/nodes/AudioProcessorNode.h"
#include "engine/nodes/AudioRouterNode.h"
#include "engine/nodes/MidiChannelSplitterNode.h"
#include "engine/nodes/MidiMonitorNode.h"
#include "engine/nodes/MidiProgramMapNode.h"
#include "engine/nodes/MidiRouterNode.h"
// #include "engine/nodes/MidiSequencerNode.h"
#include "engine/nodes/OSCReceiverNode.h"
#include "engine/nodes/OSCSenderNode.h"
#include "engine/nodes/ScriptNode.h"
#include "engine/graphnode.hpp"

#include "../nodes/mcu.hpp"

namespace element {

NodeFactory::NodeFactory()
{
    add<AudioRouterNode> (EL_NODE_ID_AUDIO_ROUTER);
    add<MidiChannelSplitterNode> (EL_NODE_ID_MIDI_CHANNEL_SPLITTER);
    add<MidiMonitorNode> (EL_NODE_ID_MIDI_MONITOR);
    add<MidiProgramMapNode> (EL_NODE_ID_MIDI_PROGRAM_MAP);
    add<MidiRouterNode> (EL_NODE_ID_MIDI_ROUTER);
    add<OSCSenderNode> (EL_NODE_ID_OSC_SENDER);
    add<OSCReceiverNode> (EL_NODE_ID_OSC_RECEIVER);
    add<ScriptNode> (EL_NODE_ID_SCRIPT);
    add<GraphNode> (EL_NODE_ID_GRAPH);
    add<MackieControlUniversal> ("el.MCU");
}

NodeFactory::~NodeFactory()
{
    knownIDs.clearQuick();
    providers.clearQuick (true);
}

//==============================================================================
void NodeFactory::getPluginDescriptions (OwnedArray<PluginDescription>& out, const String& ID, bool includeHidden)
{
    if (! includeHidden && denyIDs.contains (ID))
        return;

    for (auto* f : providers)
    {
        if (ProcessorPtr ptr = f->create (ID))
        {
            auto* desc = out.add (new PluginDescription());
            ptr->getPluginDescription (*desc);
            break;
        }
    }
}

//==============================================================================
NodeFactory& NodeFactory::add (NodeProvider* f)
{
    providers.add (f);

    denyIDs.addArray (f->getHiddenTypes());
    denyIDs.removeDuplicates (true);
    denyIDs.removeEmptyStrings();

    knownIDs.addArray (f->findTypes());
    knownIDs.removeDuplicates (true);
    knownIDs.removeEmptyStrings();
    return *this;
}

//==============================================================================
Processor* NodeFactory::instantiate (const PluginDescription& desc)
{
    return instantiate (desc.fileOrIdentifier);
}

Processor* NodeFactory::instantiate (const String& identifier)
{
    Processor* node = nullptr;
    for (const auto& f : providers)
        if (auto* const n = f->create (identifier))
        {
            node = n;
            break;
        }

    if (node)
    {
        // node->init();
    }

    return node;
}

Processor* NodeFactory::wrap (AudioProcessor* processor)
{
    jassert (processor);
    auto node = std::make_unique<AudioProcessorNode> (0, processor);

    if (node)
    {
        // init
    }

    return node.release();
}

} // namespace element

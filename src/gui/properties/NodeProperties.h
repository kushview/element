#pragma once
#include "gui/properties/MidiMultiChannelPropertyComponent.h"
#include "session/Node.h"
namespace Element {

struct NodeProperties : public Array<PropertyComponent*>
{
    NodeProperties (const Node& n, bool nodeProps = true, bool midiProps = false)
    {
        Node node = n;

        if (nodeProps)
        {
            add (new TextPropertyComponent (node.getPropertyAsValue (Tags::name), 
                "Name", 100, false, true));
        }

        if (midiProps)
        {
            // MIDI Channel
            auto* mcp = new MidiMultiChannelPropertyComponent();
            mcp->getChannelsValue().referTo (
                node.getPropertyAsValue (Tags::midiChannels, false));
            add (mcp);

            // MIDI Program

            // Key Start
            add (new SliderPropertyComponent (node.getPropertyAsValue (Tags::keyStart, false),
                "Key Start", 0.0, 127.0, 1.0));
            
            // Key End
            add (new SliderPropertyComponent (node.getPropertyAsValue (Tags::keyEnd, false),
                "Key End", 0.0, 127.0, 1.0));
            
            // Transpose
            add (new SliderPropertyComponent (node.getPropertyAsValue (Tags::transpose, false),
                "Transpose", 0.0, 127.0, 1.0));
        }
    }
};

}

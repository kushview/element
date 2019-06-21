#include "gui/nodes/VolumeNodeEditor.h"
#include "gui/NodeChannelStripComponent.h"
#include "gui/GuiCommon.h"

namespace Element {

class VolumeNodeEditor::ChannelStrip : public NodeChannelStripComponent
{
public:
    ChannelStrip (GuiController& g)
        : NodeChannelStripComponent (g, false)
    { }

    ~ChannelStrip() { }
};

VolumeNodeEditor::VolumeNodeEditor (const Node& node, GuiController& gui)
    : NodeEditorComponent (node)
{ 
    setOpaque (true);
    strip.reset (new ChannelStrip (gui));
    addAndMakeVisible (strip.get());
    setSize (40, 260);

    strip->setNode (node);
}

VolumeNodeEditor::~VolumeNodeEditor()
{
    strip.reset();
}

void VolumeNodeEditor::paint (Graphics& g) 
{
    g.fillAll (Colours::black);
}

void VolumeNodeEditor::resized()
{
    if (strip)
        strip->setBounds (getLocalBounds());
}

}
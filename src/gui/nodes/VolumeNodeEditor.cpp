#include "gui/nodes/VolumeNodeEditor.h"
#include "gui/NodeChannelStripComponent.h"
#include "gui/GuiCommon.h"
#include "engine/nodes/VolumeProcessor.h"

namespace Element {

class VolumeNodeEditor::ChannelStrip : public NodeChannelStripComponent
{
public:
    ChannelStrip (GuiController& g)
        : NodeChannelStripComponent (g, false)
    { 
        setVolumeMinMax (-30, 12, 0.5);
        
        onVolumeChanged = [this](double value)
        {
            float fvalue = static_cast<float> (value);
            if (auto object = getNode().getGraphNode()) {
                if (auto* proc = dynamic_cast<VolumeProcessor*> (object->getAudioProcessor())) {
                    if (auto* const param = dynamic_cast<AudioParameterFloat*> (proc->getParameters()[0]))
                    {
                        param->beginChangeGesture();
                        *param = fvalue;
                        param->endChangeGesture();
                    }
                }
            }
        };
    }

    ~ChannelStrip() { }

protected:
    float getCurrentVolume() override
    {
        float fvalue = 0.0f;
        if (auto object = getNode().getGraphNode())
            if (auto* proc = dynamic_cast<VolumeProcessor*> (object->getAudioProcessor()))
                if (auto* const param = dynamic_cast<AudioParameterFloat*> (proc->getParameters()[0]))
                    fvalue = *param;
        return fvalue;
    }
};

VolumeNodeEditor::VolumeNodeEditor (const Node& node, GuiController& gui)
    : NodeEditorComponent (node)
{ 
    setOpaque (true);
    strip.reset (new ChannelStrip (gui));
    addAndMakeVisible (strip.get());
    strip->setComboBoxesVisible (false, false);

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
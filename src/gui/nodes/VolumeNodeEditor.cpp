#include "gui/nodes/VolumeNodeEditor.h"
#include "gui/NodeChannelStripComponent.h"
#include "gui/GuiCommon.h"
#include "engine/nodes/VolumeProcessor.h"

namespace Element {

class VolumeNodeEditor::ChannelStrip : public NodeChannelStripComponent,
                                       public AudioProcessorParameter::Listener
{
public:
    ChannelStrip (GuiController& g)
        : NodeChannelStripComponent (g, false)
    { 
        setVolumeMinMax (-30, 12, 0.5);
        
        onVolumeChanged = [this](double value)
        {
            float fvalue = static_cast<float> (value);
            if (param != nullptr)
            {
                param->beginChangeGesture();
                *param = fvalue;
                param->endChangeGesture();
            }
        };
    }

    ~ChannelStrip()
    { 
        if (param)
            param->removeListener (this);
        param = nullptr;
        onVolumeChanged = nullptr;
    }

    void parameterValueChanged (int parameterIndex, float newValue) override
    {
        ignoreUnused (parameterIndex, newValue);
        // updateChannelStrip();
    }

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override
    {
        ignoreUnused (parameterIndex, gestureIsStarting);
    }
    
    void updateParameter()
    {
        if (param)
        {
            param->removeListener (this);
            param = nullptr;
        }

        if (auto object = getNode().getGraphNode())
            if (auto* proc = dynamic_cast<VolumeProcessor*> (object->getAudioProcessor()))
                param = dynamic_cast<AudioParameterFloat*> (proc->getParameters()[0]);
        
        stabilizeContent();

        if (param)
            param->addListener (this);
    }

protected:
    float getCurrentVolume() override
    {
        return param != nullptr ? *param : 0.f;
    }

private:
    AudioParameterFloat* param = nullptr;
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
    strip->updateParameter();
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
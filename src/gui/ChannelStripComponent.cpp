
#include "gui/ChannelStripComponent.h"

namespace Element {

ChannelStripComponent::ChannelStripComponent()
    : meter (2, false)
{
    addAndMakeVisible (fader);
    fader.setSliderStyle (Slider::LinearVertical);
    fader.setTextBoxStyle (Slider::NoTextBox, true, 1, 1);
    fader.setRange (-90.0, 12.0, 0.001);
    fader.setValue (0.f, dontSendNotification);
    fader.setSkewFactor (2);
    // fader.addListener (this);
    
    addAndMakeVisible (meter);

    addAndMakeVisible (name);
    name.setFont (name.getFont().withHeight (14));
    name.setJustificationType (Justification::centred);
    name.setText ("Name", dontSendNotification);

    addAndMakeVisible (mute);
    mute.setColour (TextButton::buttonOnColourId, Colors::toggleRed);
    mute.setButtonText ("M");
    // mute.addListener (this);

    addAndMakeVisible (volume);
    volume.setFont (volume.getFont().withHeight (12));
    volume.setJustificationType (Justification::centred);
}

ChannelStripComponent::~ChannelStripComponent() noexcept
{

}

void ChannelStripComponent::resized()
{
    auto r = getLocalBounds();
    name.setBounds (r.removeFromTop (18));

    volume.setBounds (r.removeFromBottom (18));
    auto r2 = r.removeFromBottom (18);
    mute.setBounds (r2.removeFromRight (getWidth() / 3));

    fader.setBounds (r.removeFromRight (getWidth() / 2));
    meter.setBounds (r);
}

void ChannelStripComponent::paint (Graphics&)
{

}

}
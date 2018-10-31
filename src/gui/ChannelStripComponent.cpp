
#include "gui/ChannelStripComponent.h"

namespace Element {

ChannelStripComponent::VolumeLabel::VolumeLabel() { }
ChannelStripComponent::VolumeLabel::~VolumeLabel() { }

void ChannelStripComponent::VolumeLabel::settingLabelDoubleClicked()
{
    if (auto* const strip = findParentComponentOfClass<ChannelStripComponent>())
        strip->volumeLabelDoubleClicked();
}

ChannelStripComponent::ChannelStripComponent()
    : meter (2, false)
{
    addAndMakeVisible (fader);
    fader.setSliderStyle (Slider::LinearVertical);
    fader.setTextBoxStyle (Slider::NoTextBox, true, 1, 1);
    fader.setRange (-60.0, 6.0, 0.001);
    fader.setValue (0.f, dontSendNotification);
    fader.setSkewFactor (2);
    fader.getValueObject().addListener (this);

    addAndMakeVisible (meter, 100);
    addAndMakeVisible (scale, 101);

    addAndMakeVisible (name);
    name.setFont (name.getFont().withHeight (14));
    name.setJustificationType (Justification::centred);
    name.setText ("Name", dontSendNotification);

    addAndMakeVisible (mute);
    mute.setColour (SettingButton::backgroundOnColourId, Colors::toggleBlue);
    mute.setButtonText ("M");
    mute.addListener (this);

    addAndMakeVisible (volume);
    volume.setNumDecimalPlaces (1);
    volume.setMinMax (fader.getMinimum(), fader.getMaximum());
    volume.setValue (0.0);
    volume.setTextWhenMinimum ("-inf");
    volume.getValueObject().referTo (fader.getValueObject());
    stabilizeContent();
}

ChannelStripComponent::~ChannelStripComponent() noexcept
{
    fader.getValueObject().removeListener (this);  
    volume.getValueObject().removeListener (this);
}

void ChannelStripComponent::resized()
{
    auto r1 = getLocalBounds().reduced (2);
    auto r2 = r1.removeFromRight (r1.getWidth() / 2);

    r1.removeFromTop (4);
    volume.setBounds (r1.removeFromTop (18).withSizeKeepingCentre (30, 18));
    r1.removeFromBottom (4);
    mute.setBounds (r1.removeFromBottom (18).withSizeKeepingCentre (26, 18));
    const int quarter = r2.getWidth() / 2;
    fader.setBounds (r2.removeFromRight (quarter));
    auto r3 = r2.removeFromRight (quarter);
    r3.removeFromTop (4);
    r3.removeFromBottom (4);
    meter.setBounds (r3);
    scale.setBounds (meter.getBoundsInParent());
}

void ChannelStripComponent::buttonClicked (Button* b)
{
    if (b == &mute)
    {
        mute.setToggleState (! mute.getToggleState(), false);
        powerChanged();
    }
}

void ChannelStripComponent::valueChanged (Value& value)
{
    volumeChanged (fader.getValue());
}

void ChannelStripComponent::paint (Graphics&)
{

}

void ChannelStripComponent::stabilizeContent()
{
    
}

}

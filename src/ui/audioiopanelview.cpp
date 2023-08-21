// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/node.hpp>
#include "ui/audioiopanelview.hpp"

namespace element {

//==============================================================================
AudioIOPanelView::AudioIOPanelView()
{
    setName ("audioIOPanelView");

    inputGainDial = std::make_unique<Slider> ("inputGainDial");
    addAndMakeVisible (inputGainDial.get());
    inputGainDial->setRange (-70, 12, 0);
    inputGainDial->setSliderStyle (Slider::RotaryVerticalDrag);
    inputGainDial->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    inputGainDial->setColour (Slider::rotarySliderFillColourId, Colour (0xff4ed23f));
    inputGainDial->addListener (this);

    inputGainDial->setBounds (17, 38, 48, 48);

    outputGainDial = std::make_unique<Slider> ("outputGainDial");
    addAndMakeVisible (outputGainDial.get());
    outputGainDial->setRange (-70, 12, 0);
    outputGainDial->setSliderStyle (Slider::RotaryVerticalDrag);
    outputGainDial->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    outputGainDial->setColour (Slider::thumbColourId, Colour (0xff53752b));
    outputGainDial->setColour (Slider::rotarySliderFillColourId, Colour (0xff5f8f12));
    outputGainDial->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    outputGainDial->addListener (this);

    inputGainLabel = std::make_unique<Label> ("inputGainLabel",
                                              TRANS ("Input Gain"));
    addAndMakeVisible (inputGainLabel.get());
    inputGainLabel->setFont (Font (12.00f, Font::plain).withTypefaceStyle ("Regular"));
    inputGainLabel->setJustificationType (Justification::centredLeft);
    inputGainLabel->setEditable (false, false, false);
    inputGainLabel->setColour (Label::textColourId, Colours::white);
    inputGainLabel->setColour (TextEditor::textColourId, Colours::black);
    inputGainLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    outputGainLabel = std::make_unique<Label> ("outputGainLabel",
                                               TRANS ("Output Gain"));
    addAndMakeVisible (outputGainLabel.get());
    outputGainLabel->setFont (Font (12.00f, Font::plain).withTypefaceStyle ("Regular"));
    outputGainLabel->setJustificationType (Justification::centredLeft);
    outputGainLabel->setEditable (false, false, false);
    outputGainLabel->setColour (Label::textColourId, Colours::white);
    outputGainLabel->setColour (TextEditor::textColourId, Colours::black);
    outputGainLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    inputGainDbLabel = std::make_unique<Label> ("inputGainDbLabel",
                                                TRANS ("0.00 dB"));
    addAndMakeVisible (inputGainDbLabel.get());
    inputGainDbLabel->setFont (Font (14.00f, Font::plain).withTypefaceStyle ("Regular"));
    inputGainDbLabel->setJustificationType (Justification::centredLeft);
    inputGainDbLabel->setEditable (false, false, false);
    inputGainDbLabel->setColour (Label::textColourId, Colours::white);
    inputGainDbLabel->setColour (TextEditor::textColourId, Colours::black);
    inputGainDbLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    outputGainDbLabel = std::make_unique<Label> ("outputGainDbLabel", TRANS ("0.00 dB"));
    addAndMakeVisible (outputGainDbLabel.get());
    outputGainDbLabel->setFont (Font (14.00f, Font::plain).withTypefaceStyle ("Regular"));
    outputGainDbLabel->setJustificationType (Justification::centredLeft);
    outputGainDbLabel->setEditable (false, false, false);
    outputGainDbLabel->setColour (Label::textColourId, Colours::white);
    outputGainDbLabel->setColour (TextEditor::textColourId, Colours::black);
    outputGainDbLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    nodeNameLabel = std::make_unique<Label> ("nodeNameLabel",
                                             String());
    addAndMakeVisible (nodeNameLabel.get());
    nodeNameLabel->setFont (Font (18.00f, Font::plain).withTypefaceStyle ("Regular"));
    nodeNameLabel->setJustificationType (Justification::centredLeft);
    nodeNameLabel->setEditable (false, true, false);
    nodeNameLabel->setColour (TextEditor::textColourId, Colours::black);
    nodeNameLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    nodeNameLabel->addListener (this);

    nodeNameLabel->setBounds (10, 11, 150, 24);

    setSize (220, 100);
}

AudioIOPanelView::~AudioIOPanelView()
{
    inputGainDial = nullptr;
    outputGainDial = nullptr;
    inputGainLabel = nullptr;
    outputGainLabel = nullptr;
    inputGainDbLabel = nullptr;
    outputGainDbLabel = nullptr;
    nodeNameLabel = nullptr;
}

//==============================================================================
void AudioIOPanelView::paint (Graphics& g)
{
    g.fillAll (Colour (0xff3b3b3b));
}

void AudioIOPanelView::resized()
{
    outputGainDial->setBounds ((getWidth() / 2) + 13, 38, 48, 48);
    inputGainLabel->setBounds (17 + 48 / 2 - (64 / 2), 74, 64, 24);
    outputGainLabel->setBounds (((getWidth() / 2) + 13) + 48 - 55, 74, 64, 24);
    inputGainDbLabel->setBounds (17 + 48, 50, 57, 24);
    outputGainDbLabel->setBounds (((getWidth() / 2) + 13) + 48, 50, 63, 24);
}

void AudioIOPanelView::sliderValueChanged (Slider* sliderThatWasMoved)
{
    if (sliderThatWasMoved == inputGainDial.get())
    {
        const auto dB = inputGainDial->getValue();
        String text (dB, 2);
        text << " dB";
        inputGainDbLabel->setText (text, dontSendNotification);
    }
    else if (sliderThatWasMoved == outputGainDial.get())
    {
        const auto dB = outputGainDial->getValue();
        String text (dB, 2);
        text << " dB";
        outputGainDbLabel->setText (text, dontSendNotification);
    }
}

void AudioIOPanelView::labelTextChanged (Label* labelThatHasChanged)
{
    if (labelThatHasChanged == nodeNameLabel.get())
    {
        //[UserLabelCode_nodeNameLabel] -- add your label text handling code here..
        //[/UserLabelCode_nodeNameLabel]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}

void AudioIOPanelView::setNode (const Node& node)
{
    ValueTree n = node.data();
    nodeNameLabel->getTextValue().referTo (n.getPropertyAsValue (tags::name, nullptr));
}
} // namespace element

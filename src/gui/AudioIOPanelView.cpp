/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.3.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "session/node.hpp"
using namespace Element;
//[/Headers]

#include "AudioIOPanelView.h"

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
AudioIOPanelView::AudioIOPanelView()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    setName ("audioIOPanelView");
    addAndMakeVisible (inputGainDial = new Slider ("inputGainDial"));
    inputGainDial->setRange (-70, 12, 0);
    inputGainDial->setSliderStyle (Slider::RotaryVerticalDrag);
    inputGainDial->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    inputGainDial->setColour (Slider::rotarySliderFillColourId, Colour (0xff4ed23f));
    inputGainDial->addListener (this);

    inputGainDial->setBounds (17, 38, 48, 48);

    addAndMakeVisible (outputGainDial = new Slider ("outputGainDial"));
    outputGainDial->setRange (-70, 12, 0);
    outputGainDial->setSliderStyle (Slider::RotaryVerticalDrag);
    outputGainDial->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    outputGainDial->setColour (Slider::thumbColourId, Colour (0xff53752b));
    outputGainDial->setColour (Slider::rotarySliderFillColourId, Colour (0xff5f8f12));
    outputGainDial->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    outputGainDial->addListener (this);

    addAndMakeVisible (inputGainLabel = new Label ("inputGainLabel",
                                                   TRANS ("Input Gain")));
    inputGainLabel->setFont (Font (12.00f, Font::plain).withTypefaceStyle ("Regular"));
    inputGainLabel->setJustificationType (Justification::centredLeft);
    inputGainLabel->setEditable (false, false, false);
    inputGainLabel->setColour (Label::textColourId, Colours::white);
    inputGainLabel->setColour (TextEditor::textColourId, Colours::black);
    inputGainLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (outputGainLabel = new Label ("outputGainLabel",
                                                    TRANS ("Output Gain")));
    outputGainLabel->setFont (Font (12.00f, Font::plain).withTypefaceStyle ("Regular"));
    outputGainLabel->setJustificationType (Justification::centredLeft);
    outputGainLabel->setEditable (false, false, false);
    outputGainLabel->setColour (Label::textColourId, Colours::white);
    outputGainLabel->setColour (TextEditor::textColourId, Colours::black);
    outputGainLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (inputGainDbLabel = new Label ("inputGainDbLabel",
                                                     TRANS ("0.00 dB")));
    inputGainDbLabel->setFont (Font (14.00f, Font::plain).withTypefaceStyle ("Regular"));
    inputGainDbLabel->setJustificationType (Justification::centredLeft);
    inputGainDbLabel->setEditable (false, false, false);
    inputGainDbLabel->setColour (Label::textColourId, Colours::white);
    inputGainDbLabel->setColour (TextEditor::textColourId, Colours::black);
    inputGainDbLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (outputGainDbLabel = new Label ("outputGainDbLabel",
                                                      TRANS ("0.00 dB")));
    outputGainDbLabel->setFont (Font (14.00f, Font::plain).withTypefaceStyle ("Regular"));
    outputGainDbLabel->setJustificationType (Justification::centredLeft);
    outputGainDbLabel->setEditable (false, false, false);
    outputGainDbLabel->setColour (Label::textColourId, Colours::white);
    outputGainDbLabel->setColour (TextEditor::textColourId, Colours::black);
    outputGainDbLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (nodeNameLabel = new Label ("nodeNameLabel",
                                                  String()));
    nodeNameLabel->setFont (Font (18.00f, Font::plain).withTypefaceStyle ("Regular"));
    nodeNameLabel->setJustificationType (Justification::centredLeft);
    nodeNameLabel->setEditable (false, true, false);
    nodeNameLabel->setColour (TextEditor::textColourId, Colours::black);
    nodeNameLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));
    nodeNameLabel->addListener (this);

    nodeNameLabel->setBounds (10, 11, 150, 24);

    //[UserPreSize]
    //[/UserPreSize]

    setSize (220, 100);

    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

AudioIOPanelView::~AudioIOPanelView()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    inputGainDial = nullptr;
    outputGainDial = nullptr;
    inputGainLabel = nullptr;
    outputGainLabel = nullptr;
    inputGainDbLabel = nullptr;
    outputGainDbLabel = nullptr;
    nodeNameLabel = nullptr;

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void AudioIOPanelView::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff3b3b3b));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void AudioIOPanelView::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    outputGainDial->setBounds ((getWidth() / 2) + 13, 38, 48, 48);
    inputGainLabel->setBounds (17 + 48 / 2 - (64 / 2), 74, 64, 24);
    outputGainLabel->setBounds (((getWidth() / 2) + 13) + 48 - 55, 74, 64, 24);
    inputGainDbLabel->setBounds (17 + 48, 50, 57, 24);
    outputGainDbLabel->setBounds (((getWidth() / 2) + 13) + 48, 50, 63, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void AudioIOPanelView::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == inputGainDial)
    {
        //[UserSliderCode_inputGainDial] -- add your slider handling code here..
        const auto dB = inputGainDial->getValue();
        String text (dB, 2);
        text << " dB";
        inputGainDbLabel->setText (text, dontSendNotification);
        //[/UserSliderCode_inputGainDial]
    }
    else if (sliderThatWasMoved == outputGainDial)
    {
        //[UserSliderCode_outputGainDial] -- add your slider handling code here..
        const auto dB = outputGainDial->getValue();
        String text (dB, 2);
        text << " dB";
        outputGainDbLabel->setText (text, dontSendNotification);
        //[/UserSliderCode_outputGainDial]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}

void AudioIOPanelView::labelTextChanged (Label* labelThatHasChanged)
{
    //[UserlabelTextChanged_Pre]
    //[/UserlabelTextChanged_Pre]

    if (labelThatHasChanged == nodeNameLabel)
    {
        //[UserLabelCode_nodeNameLabel] -- add your label text handling code here..
        //[/UserLabelCode_nodeNameLabel]
    }

    //[UserlabelTextChanged_Post]
    //[/UserlabelTextChanged_Post]
}

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
void AudioIOPanelView::setNode (const Node& node)
{
    ValueTree n = node.node();
    nodeNameLabel->getTextValue().referTo (n.getPropertyAsValue (Slugs::name, nullptr));
}
//[/MiscUserCode]

//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="AudioIOPanelView" componentName="audioIOPanelView"
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="16" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="220" initialHeight="100">
  <BACKGROUND backgroundColour="ff3b3b3b"/>
  <SLIDER name="inputGainDial" id="ad10f98d10a1ba0f" memberName="inputGainDial"
          virtualName="" explicitFocusOrder="0" pos="17 38 48 48" rotarysliderfill="ff4ed23f"
          min="-70.00000000000000000000" max="12.00000000000000000000"
          int="0.00000000000000000000" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1.00000000000000000000"
          needsCallback="1"/>
  <SLIDER name="outputGainDial" id="4c4d97e94f46242" memberName="outputGainDial"
          virtualName="" explicitFocusOrder="0" pos="13C 38 48 48" thumbcol="ff53752b"
          rotarysliderfill="ff5f8f12" rotaryslideroutline="66000000" min="-70.00000000000000000000"
          max="12.00000000000000000000" int="0.00000000000000000000" style="RotaryVerticalDrag"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.00000000000000000000" needsCallback="1"/>
  <LABEL name="inputGainLabel" id="578cfef930d9cb9b" memberName="inputGainLabel"
         virtualName="" explicitFocusOrder="0" pos="0Cc 74 64 24" posRelativeX="ad10f98d10a1ba0f"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="Input Gain"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="12.00000000000000000000" kerning="0.00000000000000000000"
         bold="0" italic="0" justification="33"/>
  <LABEL name="outputGainLabel" id="381cc0608cacfbbf" memberName="outputGainLabel"
         virtualName="" explicitFocusOrder="0" pos="55R 74 64 24" posRelativeX="4c4d97e94f46242"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="Output Gain"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="12.00000000000000000000" kerning="0.00000000000000000000"
         bold="0" italic="0" justification="33"/>
  <LABEL name="inputGainDbLabel" id="38e51ba306e21626" memberName="inputGainDbLabel"
         virtualName="" explicitFocusOrder="0" pos="0R 50 57 24" posRelativeX="ad10f98d10a1ba0f"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="0.00 dB"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="14.00000000000000000000" kerning="0.00000000000000000000"
         bold="0" italic="0" justification="33"/>
  <LABEL name="outputGainDbLabel" id="9037e588101840e6" memberName="outputGainDbLabel"
         virtualName="" explicitFocusOrder="0" pos="0R 50 63 24" posRelativeX="4c4d97e94f46242"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="0.00 dB"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="14.00000000000000000000" kerning="0.00000000000000000000"
         bold="0" italic="0" justification="33"/>
  <LABEL name="nodeNameLabel" id="ad398bbca3f6dfab" memberName="nodeNameLabel"
         virtualName="" explicitFocusOrder="0" pos="10 11 150 24" edTextCol="ff000000"
         edBkgCol="0" labelText="" editableSingleClick="0" editableDoubleClick="1"
         focusDiscardsChanges="0" fontname="Default font" fontsize="18.00000000000000000000"
         kerning="0.00000000000000000000" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//[EndFile] You can add extra defines here...
//[/EndFile]

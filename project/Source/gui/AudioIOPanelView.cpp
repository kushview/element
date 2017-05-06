/*
    AudioIOPanelView.cpp - This file is part of Element
    Copyright (C) 2017 Kushview, LLC.  All rights reserved.
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "AudioIOPanelView.h"


namespace Element {

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
AudioIOPanelView::AudioIOPanelView ()
{
    setName ("audioIOPanelView");
    addAndMakeVisible (inputGainDial = new Slider ("inputGainDial"));
    inputGainDial->setRange (0, 10, 0);
    inputGainDial->setSliderStyle (Slider::RotaryVerticalDrag);
    inputGainDial->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    inputGainDial->setColour (Slider::rotarySliderFillColourId, Colour (0xff4ed23f));
    inputGainDial->addListener (this);

    addAndMakeVisible (outputGainDial = new Slider ("outputGainDial"));
    outputGainDial->setRange (0, 10, 0);
    outputGainDial->setSliderStyle (Slider::RotaryVerticalDrag);
    outputGainDial->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    outputGainDial->setColour (Slider::thumbColourId, Colour (0xff53752b));
    outputGainDial->setColour (Slider::rotarySliderFillColourId, Colour (0xff5f8f12));
    outputGainDial->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66000000));
    outputGainDial->addListener (this);

    addAndMakeVisible (inputGainLabel = new Label ("inputGainLabel",
                                                   TRANS("Input Gain")));
    inputGainLabel->setFont (Font (12.00f, Font::plain));
    inputGainLabel->setJustificationType (Justification::centredLeft);
    inputGainLabel->setEditable (false, false, false);
    inputGainLabel->setColour (Label::textColourId, Colours::white);
    inputGainLabel->setColour (TextEditor::textColourId, Colours::black);
    inputGainLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (outputGainLabel = new Label ("outputGainLabel",
                                                    TRANS("Output Gain")));
    outputGainLabel->setFont (Font (12.00f, Font::plain));
    outputGainLabel->setJustificationType (Justification::centredLeft);
    outputGainLabel->setEditable (false, false, false);
    outputGainLabel->setColour (Label::textColourId, Colours::white);
    outputGainLabel->setColour (TextEditor::textColourId, Colours::black);
    outputGainLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (inputGainDbLabel = new Label ("inputGainDbLabel",
                                                     TRANS("0.00 dB")));
    inputGainDbLabel->setFont (Font (15.00f, Font::plain));
    inputGainDbLabel->setJustificationType (Justification::centredLeft);
    inputGainDbLabel->setEditable (false, false, false);
    inputGainDbLabel->setColour (Label::textColourId, Colours::white);
    inputGainDbLabel->setColour (TextEditor::textColourId, Colours::black);
    inputGainDbLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (outputGainDbLabel = new Label ("outputGainDbLabel",
                                                      TRANS("0.00 dB")));
    outputGainDbLabel->setFont (Font (15.00f, Font::plain));
    outputGainDbLabel->setJustificationType (Justification::centredLeft);
    outputGainDbLabel->setEditable (false, false, false);
    outputGainDbLabel->setColour (Label::textColourId, Colours::white);
    outputGainDbLabel->setColour (TextEditor::textColourId, Colours::black);
    outputGainDbLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


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

    inputGainDial->setBounds (7, 4, 48, 48);
    outputGainDial->setBounds ((getWidth() / 2) + 3, 4, 48, 48);
    inputGainLabel->setBounds (7 + 48 / 2 - (64 / 2), 40, 64, 24);
    outputGainLabel->setBounds (((getWidth() / 2) + 3) + 48 - 55, 40, 64, 24);
    inputGainDbLabel->setBounds (7 + 48, 16, 57, 24);
    outputGainDbLabel->setBounds (((getWidth() / 2) + 3) + 48, 16, 63, 24);
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
        //[/UserSliderCode_inputGainDial]
    }
    else if (sliderThatWasMoved == outputGainDial)
    {
        //[UserSliderCode_outputGainDial] -- add your slider handling code here..
        //[/UserSliderCode_outputGainDial]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="AudioIOPanelView" template="../../Templates/ElementTemplate.cpp"
                 componentName="audioIOPanelView" parentClasses="public Component"
                 constructorParams="" variableInitialisers="" snapPixels="16"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="0"
                 initialWidth="220" initialHeight="100">
  <BACKGROUND backgroundColour="ff3b3b3b"/>
  <SLIDER name="inputGainDial" id="ad10f98d10a1ba0f" memberName="inputGainDial"
          virtualName="" explicitFocusOrder="0" pos="7 4 48 48" rotarysliderfill="ff4ed23f"
          min="0" max="10" int="0" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <SLIDER name="outputGainDial" id="4c4d97e94f46242" memberName="outputGainDial"
          virtualName="" explicitFocusOrder="0" pos="3C 4 48 48" thumbcol="ff53752b"
          rotarysliderfill="ff5f8f12" rotaryslideroutline="66000000" min="0"
          max="10" int="0" style="RotaryVerticalDrag" textBoxPos="NoTextBox"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1"
          needsCallback="1"/>
  <LABEL name="inputGainLabel" id="578cfef930d9cb9b" memberName="inputGainLabel"
         virtualName="" explicitFocusOrder="0" pos="0Cc 40 64 24" posRelativeX="ad10f98d10a1ba0f"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="Input Gain"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="12" bold="0" italic="0" justification="33"/>
  <LABEL name="outputGainLabel" id="381cc0608cacfbbf" memberName="outputGainLabel"
         virtualName="" explicitFocusOrder="0" pos="55R 40 64 24" posRelativeX="4c4d97e94f46242"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="Output Gain"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="12" bold="0" italic="0" justification="33"/>
  <LABEL name="inputGainDbLabel" id="38e51ba306e21626" memberName="inputGainDbLabel"
         virtualName="" explicitFocusOrder="0" pos="0R 16 57 24" posRelativeX="ad10f98d10a1ba0f"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="0.00 dB"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="outputGainDbLabel" id="9037e588101840e6" memberName="outputGainDbLabel"
         virtualName="" explicitFocusOrder="0" pos="0R 16 63 24" posRelativeX="4c4d97e94f46242"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="0.00 dB"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

} /* namespace Element */

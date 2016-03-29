/*
    TransportBar.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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

//[Headers] You can add your own extra header files here...
#include "session/Session.h"
//[/Headers]

#include "TransportBar.h"


namespace Element {

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
TransportBar::TransportBar (SessionRef sess)
    : session(sess)
{
    addAndMakeVisible (play = new TextButton ("play"));
    play->setButtonText (TRANS("Play"));
    play->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    play->addListener (this);
    play->setColour (TextButton::buttonOnColourId, Colours::chartreuse);

    addAndMakeVisible (stop = new TextButton ("stop"));
    stop->setButtonText (TRANS("Stop"));
    stop->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    stop->addListener (this);

    addAndMakeVisible (record = new TextButton ("record"));
    record->setButtonText (TRANS("Rec"));
    record->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    record->addListener (this);
    record->setColour (TextButton::buttonOnColourId, Colours::red);

    addAndMakeVisible (barLabel = new Label ("barLabel",
                                             TRANS("0")));
    barLabel->setFont (Font (15.00f, Font::plain));
    barLabel->setJustificationType (Justification::centredLeft);
    barLabel->setEditable (false, false, false);
    barLabel->setColour (Label::textColourId, Colours::white);
    barLabel->setColour (TextEditor::textColourId, Colours::black);
    barLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (subLabel = new Label ("subLabel",
                                             TRANS("0")));
    subLabel->setFont (Font (15.00f, Font::plain));
    subLabel->setJustificationType (Justification::centredLeft);
    subLabel->setEditable (false, false, false);
    subLabel->setColour (Label::textColourId, Colours::white);
    subLabel->setColour (TextEditor::textColourId, Colours::black);
    subLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (beatLabel = new Label ("beatLabel",
                                              TRANS("0")));
    beatLabel->setFont (Font (15.00f, Font::plain));
    beatLabel->setJustificationType (Justification::centredLeft);
    beatLabel->setEditable (false, false, false);
    beatLabel->setColour (Label::textColourId, Colours::white);
    beatLabel->setColour (TextEditor::textColourId, Colours::black);
    beatLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (bpmSlider = new Slider ("bpmSlider"));
    bpmSlider->setRange (20, 240, 1);
    bpmSlider->setSliderStyle (Slider::IncDecButtons);
    bpmSlider->setTextBoxStyle (Slider::TextBoxRight, false, 80, 16);
    bpmSlider->addListener (this);


    //[UserPreSize]
    
    //[/UserPreSize]

    setSize (260, 16);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

TransportBar::~TransportBar()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    play = nullptr;
    stop = nullptr;
    record = nullptr;
    barLabel = nullptr;
    subLabel = nullptr;
    beatLabel = nullptr;
    bpmSlider = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void TransportBar::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void TransportBar::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    play->setBounds (225, 0, 32, 16);
    stop->setBounds (193, 0, 32, 16);
    record->setBounds (161, 0, 32, 16);
    barLabel->setBounds (89, 0, 24, 16);
    subLabel->setBounds (137, 0, 24, 16);
    beatLabel->setBounds (113, 0, 24, 16);
    bpmSlider->setBounds (0, 0, 80, 16);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void TransportBar::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == play)
    {
        //[UserButtonCode_play] -- add your button handler code here..
        play->setToggleState (! play->getToggleState(), dontSendNotification);
        session->testSetPlaying (play->getToggleState());
        //[/UserButtonCode_play]
    }
    else if (buttonThatWasClicked == stop)
    {
        //[UserButtonCode_stop] -- add your button handler code here..
        session->testSetPlaying (false);
        play->setToggleState (false, dontSendNotification);
        //[/UserButtonCode_stop]
    }
    else if (buttonThatWasClicked == record)
    {
        //[UserButtonCode_record] -- add your button handler code here..
        record->setToggleState (! record->getToggleState(), dontSendNotification);
        session->testSetRecording (record->getToggleState());
        //[/UserButtonCode_record]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void TransportBar::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == bpmSlider)
    {
        //[UserSliderCode_bpmSlider] -- add your slider handling code here..
        //[/UserSliderCode_bpmSlider]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
    void TransportBar::stabilize()
    {
        bpmSlider->getValueObject().referTo(session->getPropertyAsValue (Slugs::tempo));
    }
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TransportBar" template="../../Templates/ElementTemplate.cpp"
                 componentName="" parentClasses="public Component" constructorParams="SessionRef sess"
                 variableInitialisers="session(sess)" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="1" initialWidth="260"
                 initialHeight="16">
  <BACKGROUND backgroundColour="ffffff"/>
  <TEXTBUTTON name="play" id="b5aa83743c763018" memberName="play" virtualName=""
              explicitFocusOrder="0" pos="225 0 32 16" bgColOn="ff7fff00" buttonText="Play"
              connectedEdges="15" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="stop" id="5baf0f92435f13e3" memberName="stop" virtualName=""
              explicitFocusOrder="0" pos="193 0 32 16" buttonText="Stop" connectedEdges="15"
              needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="record" id="f6593182ab136999" memberName="record" virtualName=""
              explicitFocusOrder="0" pos="161 0 32 16" bgColOn="ffff0000" buttonText="Rec"
              connectedEdges="15" needsCallback="1" radioGroupId="0"/>
  <LABEL name="barLabel" id="383e21280d077b4a" memberName="barLabel" virtualName=""
         explicitFocusOrder="0" pos="89 0 24 16" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="0" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="subLabel" id="3d1ad303b55f2919" memberName="subLabel" virtualName=""
         explicitFocusOrder="0" pos="137 0 24 16" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="0" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="beatLabel" id="7ee24b93825298ab" memberName="beatLabel"
         virtualName="" explicitFocusOrder="0" pos="113 0 24 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="0" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <SLIDER name="bpmSlider" id="6a1f8c1a830748e6" memberName="bpmSlider"
          virtualName="" explicitFocusOrder="0" pos="0 0 80 16" min="20"
          max="240" int="1" style="IncDecButtons" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="16" skewFactor="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
void TransportBar::setBeatTime (const float t)
{
    const int bars = std::floor (t / 4.0f);
    const int beats = std::floor (t);
    barLabel->setText (String (bars + 1), dontSendNotification);
    String text = String ((beats % 4) + 1);
    beatLabel->setText (text, dontSendNotification);
    // text = String (1 + std::floor (4.f * (t - static_cast<float> (beats))));
    // beatLabel->setText (text, dontSendNotification);
}
//[/EndFile]

} /* namespace Element */

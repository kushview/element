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


    //[UserPreSize]
    //[/UserPreSize]

    setSize (168, 16);


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

    play->setBounds (136, 0, 32, 16);
    stop->setBounds (104, 0, 32, 16);
    record->setBounds (72, 0, 32, 16);
    barLabel->setBounds (0, 0, 24, 16);
    subLabel->setBounds (48, 0, 24, 16);
    beatLabel->setBounds (24, 0, 24, 16);
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



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
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
                 snapShown="1" overlayOpacity="0.330" fixedSize="1" initialWidth="168"
                 initialHeight="16">
  <BACKGROUND backgroundColour="ffffff"/>
  <TEXTBUTTON name="play" id="b5aa83743c763018" memberName="play" virtualName=""
              explicitFocusOrder="0" pos="136 0 32 16" bgColOn="ff7fff00" buttonText="Play"
              connectedEdges="15" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="stop" id="5baf0f92435f13e3" memberName="stop" virtualName=""
              explicitFocusOrder="0" pos="104 0 32 16" buttonText="Stop" connectedEdges="15"
              needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="record" id="f6593182ab136999" memberName="record" virtualName=""
              explicitFocusOrder="0" pos="72 0 32 16" bgColOn="ffff0000" buttonText="Rec"
              connectedEdges="15" needsCallback="1" radioGroupId="0"/>
  <LABEL name="barLabel" id="383e21280d077b4a" memberName="barLabel" virtualName=""
         explicitFocusOrder="0" pos="0 0 24 16" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="0" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="subLabel" id="3d1ad303b55f2919" memberName="subLabel" virtualName=""
         explicitFocusOrder="0" pos="48 0 24 16" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="0" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="beatLabel" id="7ee24b93825298ab" memberName="beatLabel"
         virtualName="" explicitFocusOrder="0" pos="24 0 24 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="0" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

} /* namespace Element */

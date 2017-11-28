/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "session/Session.h"
//[/Headers]

#include "TransportBar.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
namespace Element {
//[/MiscUserDefs]

//==============================================================================
TransportBar::TransportBar ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (play = new SettingButton ("play"));
    play->setButtonText (TRANS("Play"));
    play->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    play->addListener (this);
    play->setColour (TextButton::buttonOnColourId, Colours::chartreuse);

    addAndMakeVisible (stop = new SettingButton ("stop"));
    stop->setButtonText (TRANS("Stop"));
    stop->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    stop->addListener (this);

    addAndMakeVisible (record = new SettingButton ("record"));
    record->setButtonText (TRANS("Rec"));
    record->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    record->addListener (this);
    record->setColour (TextButton::buttonOnColourId, Colours::red);

    addAndMakeVisible (barLabel = new DragableIntLabel());
    barLabel->setName ("barLabel");

    addAndMakeVisible (beatLabel = new DragableIntLabel());
    beatLabel->setName ("beatLabel");

    addAndMakeVisible (subLabel = new DragableIntLabel());
    subLabel->setName ("subLabel");


    //[UserPreSize]
    setBeatTime (0.f);
    //[/UserPreSize]

    setSize (260, 16);


    //[Constructor] You can add your own custom stuff here..
    updateWidth();
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
    beatLabel = nullptr;
    subLabel = nullptr;


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

    play->setBounds (150, 0, 32, 16);
    stop->setBounds (115, 0, 32, 16);
    record->setBounds (80, 0, 32, 16);
    barLabel->setBounds (0, 0, 24, 16);
    beatLabel->setBounds (26, 0, 24, 16);
    subLabel->setBounds (52, 0, 24, 16);
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
        //[/UserButtonCode_play]
    }
    else if (buttonThatWasClicked == stop)
    {
        //[UserButtonCode_stop] -- add your button handler code here..
        play->setToggleState (false, dontSendNotification);
        //[/UserButtonCode_stop]
    }
    else if (buttonThatWasClicked == record)
    {
        //[UserButtonCode_record] -- add your button handler code here..
        record->setToggleState (! record->getToggleState(), dontSendNotification);
        //[/UserButtonCode_record]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void TransportBar::setBeatTime (const float t)
{
    const float beatsPerBar = 4.f;
    const int bars = std::floor (t / beatsPerBar);
    const int beats = std::floor (t);
    barLabel->tempoValue = bars + 1;
    beatLabel->tempoValue = (beats % 4) + 1;
    subLabel->tempoValue = 1;
}

void TransportBar::stabilize()
{

}

void TransportBar::updateWidth()
{
    setSize (play->getRight(), getHeight());
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TransportBar" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="260" initialHeight="16">
  <BACKGROUND backgroundColour="ffffff"/>
  <TEXTBUTTON name="play" id="b5aa83743c763018" memberName="play" virtualName="SettingButton"
              explicitFocusOrder="0" pos="150 0 32 16" bgColOn="ff7fff00" buttonText="Play"
              connectedEdges="15" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="stop" id="5baf0f92435f13e3" memberName="stop" virtualName="SettingButton"
              explicitFocusOrder="0" pos="115 0 32 16" buttonText="Stop" connectedEdges="15"
              needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="record" id="f6593182ab136999" memberName="record" virtualName="SettingButton"
              explicitFocusOrder="0" pos="80 0 32 16" bgColOn="ffff0000" buttonText="Rec"
              connectedEdges="15" needsCallback="1" radioGroupId="0"/>
  <GENERICCOMPONENT name="barLabel" id="b100c3381ab38744" memberName="barLabel" virtualName=""
                    explicitFocusOrder="0" pos="0 0 24 16" class="DragableIntLabel"
                    params=""/>
  <GENERICCOMPONENT name="beatLabel" id="ca6945d42088c4b0" memberName="beatLabel"
                    virtualName="" explicitFocusOrder="0" pos="26 0 24 16" class="DragableIntLabel"
                    params=""/>
  <GENERICCOMPONENT name="subLabel" id="715b95d9f8be65d8" memberName="subLabel" virtualName=""
                    explicitFocusOrder="0" pos="52 0 24 16" class="DragableIntLabel"
                    params=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...


} /* namespace element */

//[/EndFile]

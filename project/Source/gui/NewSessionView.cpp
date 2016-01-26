/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 4.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "NewSessionView.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
NewSessionView::NewSessionView (Session& s)
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    setName ("NewSessionView");
    addAndMakeVisible (properties = new PropertyPanel());
    properties->setName ("properties");


    //[UserPreSize]
    //[/UserPreSize]

    setSize (300, 200);


    //[Constructor] You can add your own custom stuff here..
    Array<PropertyComponent*> props;
    props.add (new TextPropertyComponent (s.getPropertyAsValue ("name"), "Session Name", 64, false));
    props.add (new SliderPropertyComponent (s.getPropertyAsValue ("tempo"), "Tempo", 20.0f, 400.f, 1.0f));
    properties->addProperties (props);
    //[/Constructor]
}

NewSessionView::~NewSessionView()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    properties = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void NewSessionView::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void NewSessionView::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    properties->setBounds (8, 8, getWidth() - 16, getHeight() - 16);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="NewSessionView" template="../../../project/Templates/BTV.cpp"
                 componentName="NewSessionView" parentClasses="public Component"
                 constructorParams="Session&amp; s" variableInitialisers="" snapPixels="8"
                 snapActive="1" snapShown="1" overlayOpacity="0.330" fixedSize="0"
                 initialWidth="300" initialHeight="200">
  <BACKGROUND backgroundColour="ffffff"/>
  <GENERICCOMPONENT name="properties" id="2eacb3ed761b86bb" memberName="properties"
                    virtualName="" explicitFocusOrder="0" pos="8 8 16M 16M" class="PropertyPanel"
                    params=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

/*
    NewSessionView.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

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
//[/Headers]

#include "NewSessionView.h"


namespace Element {

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
NewSessionView::NewSessionView (Session& s)
{
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

<JUCER_COMPONENT documentType="Component" className="NewSessionView" template="../../Templates/ElementTemplate.cpp"
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

} /* namespace Element */

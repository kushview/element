/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.1.2

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "session/CommandManager.h"
//[/Headers]

#include "AboutComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
namespace Element {
//[/MiscUserDefs]

//==============================================================================
AboutComponent::AboutComponent ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    setName ("AboutComponent");
    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("Element")));
    label->setFont (Font ("Arial Black", 86.50f, Font::bold));
    label->setJustificationType (Justification::centred);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colours::whitesmoke);
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label2 = new Label ("new label",
                                           TRANS("Element v0.1.1 (build 1)")));
    label2->setFont (Font (14.00f, Font::plain).withTypefaceStyle ("Regular"));
    label2->setJustificationType (Justification::centredLeft);
    label2->setEditable (false, false, false);
    label2->setColour (Label::textColourId, Colour (0xffe6e6e6));
    label2->setColour (TextEditor::textColourId, Colours::black);
    label2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]
    String versionText = ProjectInfo::projectName;
    versionText << " v" << ProjectInfo::versionString << " (build 1)";
    label2->setText (versionText, dontSendNotification);
    label2->setInterceptsMouseClicks(false, true);
    label->setInterceptsMouseClicks(false, true);
    //[/UserPreSize]

    setSize (524, 460);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

AboutComponent::~AboutComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    label = nullptr;
    label2 = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void AboutComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colours::black);

    {
        int x = 0, y = 0, width = 524, height = 200;
        Colour fillColour = Colour (0xff4765a0);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.fillRect (x, y, width, height);
    }

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void AboutComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    label->setBounds ((getWidth() / 2) - (302 / 2), 40, 302, 72);
    label2->setBounds (8, 208, 224, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void AboutComponent::mouseDown (const MouseEvent& e)
{
    //[UserCode_mouseDown] -- Add your code here...
    // FIXME:
//    if (isOnDesktop() || isVisible())
//        gui.commander().invokeDirectly (Commands::showAbout, true);
    //[/UserCode_mouseDown]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="AboutComponent" componentName="AboutComponent"
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="524" initialHeight="460">
  <METHODS>
    <METHOD name="mouseDown (const MouseEvent&amp; e)"/>
  </METHODS>
  <BACKGROUND backgroundColour="ff000000">
    <RECT pos="0 0 524 200" fill="solid: ff4765a0" hasStroke="0"/>
  </BACKGROUND>
  <LABEL name="new label" id="c1f504802c508a9" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="0Cc 40 302 72" textCol="fff5f5f5"
         edTextCol="ff000000" edBkgCol="0" labelText="Element" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Arial Black"
         fontsize="86.5" kerning="0" bold="1" italic="0" justification="36"
         typefaceStyle="Bold"/>
  <LABEL name="new label" id="8a84133becd36d77" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="8 208 224 24" textCol="ffe6e6e6"
         edTextCol="ff000000" edBkgCol="0" labelText="Element v0.1.1 (build 1)"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="14" kerning="0" bold="0" italic="0"
         justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
} /* namespace Element */
//[/EndFile]

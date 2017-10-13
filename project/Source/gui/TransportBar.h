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

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "ElementApp.h"
#include "session/Session.h"

namespace Element {
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class TransportBar  : public Component,
                      public Button::Listener,
                      public Slider::Listener
{
public:
    //==============================================================================
    TransportBar (SessionRef sess);
    ~TransportBar();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setBeatTime (const float t);
    void stabilize();
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    SessionRef session;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<TextButton> play;
    ScopedPointer<TextButton> stop;
    ScopedPointer<TextButton> record;
    ScopedPointer<Label> barLabel;
    ScopedPointer<Label> subLabel;
    ScopedPointer<Label> beatLabel;
    ScopedPointer<Slider> bpmSlider;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
};

//[EndFile] You can add extra defines here...
} /* namespace element */
//[/EndFile]

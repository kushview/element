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

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "ElementApp.h"

namespace element {
class Node;

//[/Headers]

//==============================================================================
/**
                                                                    //[Comments]
                                                                    //[/Comments]
*/
class AudioIOPanelView : public Component,
                         public Slider::Listener,
                         public Label::Listener
{
public:
    //==============================================================================
    AudioIOPanelView();
    ~AudioIOPanelView();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setNode (const element::Node& node);
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void labelTextChanged (Label* labelThatHasChanged) override;

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    Value inputGain, outputGain;
    //[/UserVariables]

    //==============================================================================
    std::unique_ptr<Slider> inputGainDial;
    std::unique_ptr<Slider> outputGainDial;
    std::unique_ptr<Label> inputGainLabel;
    std::unique_ptr<Label> outputGainLabel;
    std::unique_ptr<Label> inputGainDbLabel;
    std::unique_ptr<Label> outputGainDbLabel;
    std::unique_ptr<Label> nodeNameLabel;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioIOPanelView)
};

//[EndFile] You can add extra defines here...
} // namespace element
//[/EndFile]

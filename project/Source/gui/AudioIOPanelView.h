/*
    AudioIOPanelView.h - This file is part of Element
    Copyright (C) 2017 Kushview, LLC.  All rights reserved.
*/

#ifndef __JUCE_HEADER_2EFBBB6231C674B2__
#define __JUCE_HEADER_2EFBBB6231C674B2__

//[Headers]     -- You can add your own extra header files here --
#include "../../JuceLibraryCode/JuceHeader.h"
//[/Headers]



namespace Element {

/**
                                                                    //[Comments]
                                                                    //[/Comments]
*/
class AudioIOPanelView  : public Component,
                          public SliderListener
{
public:
    //==============================================================================
    AudioIOPanelView ();
    ~AudioIOPanelView();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Slider> inputGainDial;
    ScopedPointer<Slider> outputGainDial;
    ScopedPointer<Label> inputGainLabel;
    ScopedPointer<Label> outputGainLabel;
    ScopedPointer<Label> inputGainDbLabel;
    ScopedPointer<Label> outputGainDbLabel;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioIOPanelView)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

} /* namespace Element */

#endif   // __JUCE_HEADER_2EFBBB6231C674B2__

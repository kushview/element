/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_EEC99E9977A4A1D4__
#define __JUCE_HEADER_EEC99E9977A4A1D4__

//[Headers]     -- You can add your own extra header files here --
#include <element/Juce.h>

namespace Element {

    class Session;

}
//[/Headers]



namespace Element {
namespace Gui {
//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class TransportBar  : public Component,
                      public ButtonListener
{
public:
    //==============================================================================
    TransportBar (Session& sess);
    ~TransportBar();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    Session& session;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<TextButton> play;
    ScopedPointer<TextButton> stop;
    ScopedPointer<TextButton> seekZero;
    ScopedPointer<TextButton> stepForward;
    ScopedPointer<TextButton> stepBack;
    ScopedPointer<TextButton> record;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

}} /* namespace Element::Gui */

#endif   // __JUCE_HEADER_EEC99E9977A4A1D4__

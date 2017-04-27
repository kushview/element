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

#ifndef __JUCE_HEADER_826E9038040E8C13__
#define __JUCE_HEADER_826E9038040E8C13__

//[Headers]     -- You can add your own extra header files here --
#include "ElementApp.h"

namespace Element {
namespace Gui {
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class AOkCancelAlertButtonBar  : public Component,
                                 public ButtonListener
{
public:
    //==============================================================================
    AOkCancelAlertButtonBar ();
    ~AOkCancelAlertButtonBar();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);

    // Binary resources:
    static const char* buttonokout_png;
    static const int buttonokout_pngSize;
    static const char* buttonokmouseover_png;
    static const int buttonokmouseover_pngSize;
    static const char* buttonokin_png;
    static const int buttonokin_pngSize;
    static const char* buttoncancelin_png;
    static const int buttoncancelin_pngSize;
    static const char* buttoncancelmouseover_png;
    static const int buttoncancelmouseover_pngSize;
    static const char* buttoncancelout_png;
    static const int buttoncancelout_pngSize;


private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<ImageButton> fOkButton;
    ScopedPointer<ImageButton> fCancelButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AOkCancelAlertButtonBar)
};

//[EndFile] You can add extra defines here...
}}
//[/EndFile]

#endif   // __JUCE_HEADER_826E9038040E8C13__

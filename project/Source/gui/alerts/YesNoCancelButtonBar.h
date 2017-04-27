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

#ifndef __JUCE_HEADER_AD8B3DC21171FFC3__
#define __JUCE_HEADER_AD8B3DC21171FFC3__

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
class AYesNoCancelButtonBar  : public Component,
                               public ButtonListener
{
public:
    //==============================================================================
    AYesNoCancelButtonBar ();
    ~AYesNoCancelButtonBar();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);

    // Binary resources:
    static const char* buttonyesin_png;
    static const int buttonyesin_pngSize;
    static const char* buttonyesmouseover_png;
    static const int buttonyesmouseover_pngSize;
    static const char* buttonyesout_png;
    static const int buttonyesout_pngSize;
    static const char* buttonnoin_png;
    static const int buttonnoin_pngSize;
    static const char* buttonnomouseover_png;
    static const int buttonnomouseover_pngSize;
    static const char* buttonnoout_png;
    static const int buttonnoout_pngSize;
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
    ScopedPointer<ImageButton> fCancelButton;
    ScopedPointer<ImageButton> fNoButton;
    ScopedPointer<ImageButton> fYesButton;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AYesNoCancelButtonBar)
};

//[EndFile] You can add extra defines here...
}}
//[/EndFile]

#endif   // __JUCE_HEADER_AD8B3DC21171FFC3__

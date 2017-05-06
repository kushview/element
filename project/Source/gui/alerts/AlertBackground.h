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

#ifndef __JUCE_HEADER_32BB421823BE08F9__
#define __JUCE_HEADER_32BB421823BE08F9__

//[Headers]     -- You can add your own extra header files here --
#include "ElementApp.h"
namespace Element {
namespace Gui {
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Jucer.

    This class is mostly for containing the image resources which are drawn in the
    background of our alerts. \see Alerts.h. Its a component so we can use the
    jucer to crete the graphic resources. It won't work if you instantiate it
    as a Component. Just call DrawBackground from whatever class you want these
    images to show up in.

                                                                    //[/Comments]
*/
class AAlertBackground  : public Component
{
public:
    //==============================================================================
    AAlertBackground ();
    ~AAlertBackground();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    static void DrawBackground(Graphics& g, int x, int y,  int width, int height,
                               AlertWindow::AlertIconType type);
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();

    // Binary resources:
    static const char* backgroundbeatthang_png;
    static const int backgroundbeatthang_pngSize;
    static const char* backgrounderror_png;
    static const int backgrounderror_pngSize;
    static const char* backgroundquestion_png;
    static const int backgroundquestion_pngSize;


private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    Image cachedImage_backgroundbeatthang_png;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AAlertBackground)
};

//[EndFile] You can add extra defines here...
}}
//[/EndFile]

#endif   // __JUCE_HEADER_32BB421823BE08F9__

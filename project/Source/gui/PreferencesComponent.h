/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 4.3.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_248C56742F074362__
#define __JUCE_HEADER_248C56742F074362__

//[Headers]     -- You can add your own extra header files here --
#include <element/Juce.h>

namespace Element {
class GuiApp;

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PreferencesComponent  : public Component
{
public:
    //==============================================================================
    PreferencesComponent (GuiApp& gui_);
    ~PreferencesComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setPage (const String& uri);

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    GuiApp& gui;
    class PageList;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<PageList> pageList;
    ScopedPointer<GroupComponent> groupComponent;
    ScopedPointer<Component> pageComponent;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreferencesComponent)
};

//[EndFile] You can add extra defines here...
}
//[/EndFile]

#endif   // __JUCE_HEADER_248C56742F074362__

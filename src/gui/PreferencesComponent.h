/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "ElementApp.h"

namespace element {
class GuiController;
//[/Headers]

//==============================================================================
/**
                                                                    //[Comments]
                                                                    //[/Comments]
*/
class PreferencesComponent : public Component
{
public:
    //==============================================================================
    PreferencesComponent (Context& g, GuiController& _gui);
    ~PreferencesComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setPage (const String& name);
    void addPage (const String& name);
    Component* createPageForName (const String& name);
    void updateSize();
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    class PageList;
    Context& world;
    GuiController& gui;
    OwnedArray<Component> pages;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<PageList> pageList;
    ScopedPointer<GroupComponent> groupComponent;
    ScopedPointer<Component> pageComponent;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreferencesComponent)
};

//[EndFile] You can add extra defines here...
} // namespace element
//[/EndFile]

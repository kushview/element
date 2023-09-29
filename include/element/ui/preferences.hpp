// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/gui_basics.hpp>

#define EL_GENERAL_SETTINGS_NAME "General"
#define EL_AUDIO_SETTINGS_NAME "Audio"
#define EL_MIDI_SETTINGS_NAME "MIDI"
#define EL_OSC_SETTINGS_NAME "OSC"
#define EL_PLUGINS_PREFERENCE_NAME "Plugins"
#define EL_REPOSITORY_PREFERENCE_NAME "Updates"

namespace element {

class Context;
class GuiService;
class SettingsPage;

//==============================================================================
/** The main preferences UI component. */
class Preferences : public juce::Component
{
public:
    Preferences (GuiService& ui);
    virtual ~Preferences();

    /** Creates and adds the default pages. */
    void addDefaultPages();

    /** Set the current page
        
        @param name The page to change to.
    */
    void setPage (const juce::String& name);

    /** Update the size. Ensures correct size. */
    void updateSize();

    /** @internal */
    void paint (juce::Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    bool keyPressed (const juce::KeyPress& key) override;

protected:
    //==============================================================================
    void addPage (const juce::String& name);
    virtual juce::Component* createPageForName (const juce::String& name);

private:
    Context& _context;
    GuiService& _ui;

    //==============================================================================
    class PageList;
    std::unique_ptr<PageList> pageList;
    std::unique_ptr<juce::Component> pageComponent;
    juce::OwnedArray<juce::Component> pages;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Preferences)
};

}

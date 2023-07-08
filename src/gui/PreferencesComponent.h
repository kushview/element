// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/gui_basics.hpp>

namespace element {
class GuiService;
class SettingsPage;

//==============================================================================
class PreferencesComponent : public juce::Component
{
public:
    //==============================================================================
    PreferencesComponent (GuiService& ui);
    ~PreferencesComponent();

    void setPage (const juce::String& name);
    void addPage (const juce::String& name);

    void updateSize();

    void paint (juce::Graphics& g) override;
    void resized() override;

    bool keyPressed (const KeyPress& key) override;

private:
    Context& _context;
    GuiService& _ui;

    //==============================================================================
    class PageList;
    std::unique_ptr<PageList> pageList;
    std::unique_ptr<Component> pageComponent;
    juce::OwnedArray<juce::Component> pages;

    //==============================================================================
    Component* createPageForName (const String& name);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreferencesComponent)
};

using Preferences = PreferencesComponent;

} // namespace element

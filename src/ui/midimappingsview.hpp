// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/ui/content.hpp>

// Keep the legacy view identifier so existing settings/commands resolve.
#define EL_VIEW_CONTROLLERS "ControllersView"

namespace element {

/** A flat table of the session's MIDI mappings plus Learn/Delete actions.
    Replaces the legacy ControllersView + ControllerMapsView. */
class MidiMappingsView : public ContentView,
                         public juce::TableListBoxModel,
                         public juce::Button::Listener
{
public:
    MidiMappingsView();
    ~MidiMappingsView() override;

    void resized() override;
    void didBecomeActive() override { stabilizeContent(); }
    void stabilizeContent() override;

    //=========================================================================
    int getNumRows() override;
    void paintRowBackground (juce::Graphics&, int row, int w, int h, bool selected) override;
    void paintCell (juce::Graphics&, int row, int columnId, int w, int h, bool selected) override;

    //=========================================================================
    void buttonClicked (juce::Button*) override;

private:
    juce::TableListBox table;
    juce::TextButton learnButton { "Learn" };
    juce::TextButton deleteButton { "Delete" };

    void updateLearnButton();
};

} // namespace element

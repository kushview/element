// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include <element/ui/content.hpp>

// Keep the legacy view identifier so existing settings/commands resolve.
#define EL_VIEW_CONTROLLERS "ControllersView"

namespace element {

class MidiMappingProperties;

/** A flat table of the session's MIDI mappings with a properties panel for the
    selected row, plus a Delete action.
    Replaces the legacy ControllersView + ControllerMapsView. */
class MidiMappingsView : public ContentView,
                         public juce::TableListBoxModel,
                         public juce::Button::Listener,
                         public juce::Value::Listener
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
    void sortOrderChanged (int newSortColumnId, bool isForwards) override;
    void selectedRowsChanged (int lastRowSelected) override;
    void deleteKeyPressed (int lastRowSelected) override;
    void cellClicked (int rowNumber, int columnId, const juce::MouseEvent&) override;

    //=========================================================================
    void buttonClicked (juce::Button*) override;

    //=========================================================================
    /** A filter dropdown changed; re-filter the list. */
    void valueChanged (juce::Value&) override;

private:
    juce::TableListBox table;
    std::unique_ptr<MidiMappingProperties> props;
    juce::StretchableLayoutManager layout;
    std::unique_ptr<juce::StretchableLayoutResizerBar> resizer;
    juce::TextButton deleteButton { "Delete" };
    juce::Label emptyLabel;

    /** List filters (empty = show all). Shared with the panel's Filter section;
        applied in updateRowOrder(). */
    juce::Value filterDevice, filterNode, filterGraph, filterTarget;

    /** Display row -> index into the session's mapping list, honouring the
        current filter + sort column. Rebuilt by updateRowOrder(). */
    std::vector<int> rowOrder;
    void updateRowOrder();
    int mappedRow (int row) const;

    /** Rebuild live engine bindings after an in-place edit and refresh the table. */
    void mappingEdited();

    /** Remove the mapping shown at the given display row and refresh. */
    void removeMapping (int displayRow);
};

} // namespace element

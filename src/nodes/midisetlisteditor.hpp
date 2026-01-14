// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/ui/nodeeditor.hpp>
#include "nodes/midisetlist.hpp"

namespace element {

class MidiSetListEditor : public NodeEditor,
                          public ChangeListener
{
public:
    MidiSetListEditor (const Node& node);
    virtual ~MidiSetListEditor();

    void paint (Graphics&) override;
    void resized() override;

    void addProgram();
    void removeSelectedProgram();
    int getNumPrograms() const;
    MidiSetListProcessor::ProgramEntry getProgram (int) const;
    void setProgram (int, MidiSetListProcessor::ProgramEntry);
    void sendProgram (int);

    void setFontControlsVisible (bool);

    float getDefaultFontSize() const { return 15.f; }
    float getFontSize() const { return fontSize; }
    void setFontSize (float newSize, bool updateNode = true);

    void selectRow (int row);
    void setStoreSize (const bool storeSize);

    void changeListenerCallback (ChangeBroadcaster*) override;

    bool keyPressed (const KeyPress&) override;

private:
    Node node;
    class TableModel;
    friend class TableModel;
    std::unique_ptr<TableModel> model;
    TableListBox table;
    TextButton addButton;
    TextButton delButton;
    Slider fontSlider;
    bool storeSizeInNode = true;
    float fontSize = 15.f;
    SignalConnection lastProgramChangeConnection;
    void selectLastProgram();
    void updateTableHeaderSizes();
};

} // namespace element

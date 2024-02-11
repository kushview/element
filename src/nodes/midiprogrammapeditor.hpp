// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/nodeeditor.hpp>
#include "nodes/midiprogrammap.hpp"

namespace element {

class MidiProgramMapEditor : public NodeEditor,
                             public ChangeListener
{
public:
    MidiProgramMapEditor (const Node& node);
    virtual ~MidiProgramMapEditor();

    void paint (Graphics&) override;
    void resized() override;

    void addProgram();
    void removeSelectedProgram();
    int getNumPrograms() const;
    MidiProgramMapNode::ProgramEntry getProgram (int) const;
    void setProgram (int, MidiProgramMapNode::ProgramEntry);
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

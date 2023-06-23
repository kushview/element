/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include <element/ui/nodeeditor.hpp>
#include "engine/nodes/MidiProgramMapNode.h"

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
};

} // namespace element

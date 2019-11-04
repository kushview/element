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

#include "gui/nodes/NodeEditorComponent.h"
#include "engine/nodes/MidiMonitorNode.h"

namespace Element {

class MidiMonitorLogListBox : public ListBox,
                              private ListBoxModel,
                              private AsyncUpdater
{
public:
    MidiMonitorLogListBox()
    {
        setModel (this);
    }

    ~MidiMonitorLogListBox() override = default;

    int getNumRows() override
    {
        return logList.size();
    }

    void paintListBoxItem (int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        ignoreUnused (rowIsSelected);
        if (isPositiveAndBelow (row, logList.size()))
        {
            g.setColour (Colours::white);
            g.drawText (logList[row],
                        Rectangle<int> (width, height).reduced (4, 0),
                        Justification::centredLeft, true);
        }
    }

    void addMessage (const String& message)
    {
        logList.add (message);
        triggerAsyncUpdate();
    }

    void clear()
    {
        logList.clear();
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        updateContent();
        scrollToEnsureRowIsOnscreen (logList.size() - 1);
        repaint();
    }

private:
    StringArray logList;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiMonitorLogListBox)
};


class MidiMonitorNodeEditor : public NodeEditorComponent,
                              public ChangeListener
{
public:

    MidiMonitorNodeEditor (const Node& node);
    virtual ~MidiMonitorNodeEditor() {};

    void paint (Graphics&) override {};
    void resized() override {};
    void changeListenerCallback (ChangeBroadcaster*) override;

private:
    Node node;
    MidiMonitorLogListBox midiMonitorLog;
};

}

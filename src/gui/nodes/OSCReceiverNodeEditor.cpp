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

#include "engine/nodes/OSCReceiverNode.h"
#include "gui/nodes/OSCReceiverNodeEditor.h"
#include "gui/ViewHelpers.h"

namespace Element {

typedef ReferenceCountedObjectPtr<OSCReceiverNode> OSCReceiverNodePtr;

OSCReceiverNodeEditor::OSCReceiverNodeEditor (const Node& node)
    : NodeEditorComponent (node)
{
    portNumberLabel.setBounds (10, 18, 130, 25);
    addAndMakeVisible (portNumberLabel);

    portNumberField.setEditable (true, true, true);
    portNumberField.setBounds (140, 18, 50, 25);
    addAndMakeVisible (portNumberField);

    connectButton.setBounds (200, 18, 100, 25);
    addAndMakeVisible (connectButton);
    connectButton.onClick = [this] { connectButtonClicked(); };

    clearButton.setBounds (310, 18, 60, 25);
    addAndMakeVisible (clearButton);
    clearButton.onClick = [this] { clearButtonClicked(); };

    connectionStatusLabel.setBounds (390, 18, 100, 25);
    updateConnectionStatusLabel();
    addAndMakeVisible (connectionStatusLabel);

    oscReceiverLog.setBounds (0, 60, 700, 340);
    addAndMakeVisible (oscReceiverLog);

    oscReceiver.addListener (this);
}

OSCReceiverNodeEditor::~OSCReceiverNodeEditor()
{
}
/*
void OSCReceiverNodeEditor::timerCallback ()
{
    if (OSCReceiverNodePtr node = getNodeObjectOfType<OSCReceiverNode>())
    {
        if (node->numSamples.get() <= 0)
            return;

        MidiBuffer midi;
        node->getMessages(midi);

        MidiBuffer::Iterator iter1 (midi);
        MidiMessage msg;
        int frame;

        while (iter1.getNextEvent (msg, frame))
            oscReceiverLog.addMessage (msg.getDescription());
    }
}
*/

void OSCReceiverNodeEditor::resized ()
{
    oscReceiverLog.setBounds (getLocalBounds().reduced (4));
}

};

/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.
    Author Eliot Akira <me@eliotakira.com>
    
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

#include "engine/nodes/MidiMonitorNode.h"
#include "gui/nodes/MidiMonitorNodeEditor.h"
#include "gui/ViewHelpers.h"

namespace element {

using MidiMonitorNodePtr = ReferenceCountedObjectPtr<MidiMonitorNode>;

class MidiMonitorNodeEditor::Logger : public ListBox,
                                      public ListBoxModel,
                                      public AsyncUpdater
{
public:
    /** Constructor */
    Logger (MidiMonitorNodePtr n)
        : node (n)
    {
        jassert (node != nullptr);
        setModel (this);
        connection = node->messagesLogged.connect (
            std::bind (&Logger::triggerAsyncUpdate, this));
    }

    /** Destructor */
    ~Logger()
    {
        connection.disconnect();
        node = nullptr;
    }

    int getNumRows() override { return node->logger().size(); }

    void paintListBoxItem (int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        const auto& logList = node->logger();
        ignoreUnused (rowIsSelected);
        g.setFont (Font (Font::getDefaultMonospacedFontName(),
                         g.getCurrentFont().getHeight(),
                         Font::plain));
        if (isPositiveAndBelow (row, logList.size()))
            ViewHelpers::drawBasicTextRow (logList[row], g, width, height, false);
    }

    void handleAsyncUpdate() override
    {
        const auto& logList = node->logger();
        updateContent();
        scrollToEnsureRowIsOnscreen (logList.size() - 1);
        repaint();
    }

private:
    MidiMonitorNodePtr node;
    SignalConnection connection;
};

MidiMonitorNodeEditor::MidiMonitorNodeEditor (const Node& node)
    : NodeEditor (node)
{
    setOpaque (true);
    logger.reset (new Logger (getNodeObjectOfType<MidiMonitorNode>()));
    addAndMakeVisible (logger.get());

    addAndMakeVisible (clearButton);
    clearButton.setButtonText ("Clear");
    clearButton.onClick = [this]() {
        if (auto* n = getNodeObjectOfType<MidiMonitorNode>())
            n->clearMessages();
    };

    setSize (320, 160);
}

MidiMonitorNodeEditor::~MidiMonitorNodeEditor()
{
    logger.reset();
}

void MidiMonitorNodeEditor::resized()
{
    auto r1 = getLocalBounds().reduced (4);
    clearButton.changeWidthToFitText (24);
    clearButton.setBounds (r1.getX(), r1.getY(), clearButton.getWidth(), clearButton.getHeight());
    r1.removeFromTop (24 + 2);
    logger->setBounds (r1);
}

}; // namespace element

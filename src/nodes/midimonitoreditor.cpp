// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later
// Author: Eliot Akira <me@eliotakira.com>

#include "nodes/midimonitor.hpp"
#include "nodes/midimonitoredtor.hpp"
#include "ui/viewhelpers.hpp"

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

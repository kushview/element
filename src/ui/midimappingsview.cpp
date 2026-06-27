// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/midimappingsview.hpp"
#include "ui/viewhelpers.hpp"
#include <element/ui/content.hpp>
#include <element/session.hpp>
#include <element/midimapping.hpp>
#include <element/node.hpp>
#include "services/mappingservice.hpp"

namespace element {

using namespace juce;

namespace {
enum Columns
{
    ColDevice = 1,
    ColEvent,
    ColChannel,
    ColTarget
};
}

MidiMappingsView::MidiMappingsView()
{
    setName ("MidiMappings");

    addAndMakeVisible (table);
    table.setModel (this);
    table.getHeader().addColumn ("Device", ColDevice, 160);
    table.getHeader().addColumn ("Event", ColEvent, 100);
    table.getHeader().addColumn ("Ch", ColChannel, 50);
    table.getHeader().addColumn ("Target", ColTarget, 220);
    table.setHeaderHeight (22);

    addAndMakeVisible (learnButton);
    learnButton.addListener (this);
    addAndMakeVisible (deleteButton);
    deleteButton.addListener (this);

    setEscapeTriggersClose (true);
}

MidiMappingsView::~MidiMappingsView()
{
    learnButton.removeListener (this);
    deleteButton.removeListener (this);
    table.setModel (nullptr);
}

void MidiMappingsView::resized()
{
    auto r = getLocalBounds().reduced (4);
    auto top = r.removeFromTop (26);
    learnButton.setBounds (top.removeFromLeft (80));
    top.removeFromLeft (4);
    deleteButton.setBounds (top.removeFromLeft (80));
    r.removeFromTop (4);
    table.setBounds (r);
}

void MidiMappingsView::stabilizeContent()
{
    table.updateContent();
    table.repaint();
    updateLearnButton();
}

void MidiMappingsView::updateLearnButton()
{
    bool learning = false;
    if (auto* cc = ViewHelpers::findContentComponent (this))
        if (auto* maps = cc->services().find<MappingService>())
            learning = maps->isLearning();
    learnButton.setToggleState (learning, dontSendNotification);
}

int MidiMappingsView::getNumRows()
{
    if (auto session = ViewHelpers::getSession (this))
        return session->getNumMidiMappings();
    return 0;
}

void MidiMappingsView::paintRowBackground (Graphics& g, int, int, int, bool selected)
{
    if (selected)
        g.fillAll (Colours::darkgrey.withAlpha (0.4f));
}

void MidiMappingsView::paintCell (Graphics& g, int row, int columnId, int w, int h, bool)
{
    auto session = ViewHelpers::getSession (this);
    if (session == nullptr || ! isPositiveAndBelow (row, session->getNumMidiMappings()))
        return;

    auto mapping = session->getMidiMapping (row);
    String text;

    switch (columnId)
    {
        case ColDevice:
            text = mapping.getDevice().isEmpty() ? "(any)" : mapping.getDevice();
            break;
        case ColEvent:
            text = (mapping.isNoteEvent() ? "Note " : "CC ") + String (mapping.getEventId());
            break;
        case ColChannel:
            text = mapping.getMidiChannel() == 0 ? "omni" : String (mapping.getMidiChannel());
            break;
        case ColTarget: {
            auto node = session->findNodeById (mapping.getNodeUuid());
            const auto nodeName = node.isValid() ? node.getName() : String ("(missing)");
            text = nodeName + " : param " + String (mapping.getParameterIndex());
            break;
        }
        default:
            break;
    }

    g.setColour (Colours::lightgrey);
    g.drawText (text, 4, 0, w - 6, h, Justification::centredLeft);
}

void MidiMappingsView::buttonClicked (Button* b)
{
    auto* cc = ViewHelpers::findContentComponent (this);
    if (cc == nullptr)
        return;
    auto* maps = cc->services().find<MappingService>();
    if (maps == nullptr)
        return;

    if (b == &learnButton)
    {
        maps->learn (! maps->isLearning());
        updateLearnButton();
    }
    else if (b == &deleteButton)
    {
        const int row = table.getSelectedRow();
        if (auto session = ViewHelpers::getSession (this))
            if (isPositiveAndBelow (row, session->getNumMidiMappings()))
                maps->remove (session->getMidiMapping (row));
        stabilizeContent();
    }
}

} // namespace element

// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/midimappingsview.hpp"
#include "ui/viewhelpers.hpp"
#include <element/juce/audio_devices.hpp>
#include <element/ui/content.hpp>
#include <element/session.hpp>
#include <element/midimapping.hpp>
#include <element/node.hpp>
#include <element/parameter.hpp>
#include <element/processor.hpp>
#include "services/mappingservice.hpp"

namespace element {

using namespace juce;

namespace {
enum Columns
{
    ColDevice = 1,
    ColEvent,
    ColChannel,
    ColNode,
    ColParameter
};

/** Resolve a stored JUCE MIDI input identifier to its human-readable name.
    Falls back to "(any)" for the wildcard device, or to the raw identifier
    if the device is not currently connected. */
String deviceName (const String& identifier)
{
    if (identifier.isEmpty())
        return "(any)";
    for (const auto& info : MidiInput::getAvailableDevices())
        if (info.identifier == identifier)
            return info.name;
    return identifier;
}

/** Resolve a mapping's target parameter to a display name, handling the
    special Enabled/Bypass/Mute/Gain indices and falling back to the 1-based
    parameter number when no name is available. */
String parameterName (const Node& node, int index)
{
    if (Processor::isSpecialParameter (index))
        return Processor::getSpecialParameterName (index);

    if (auto* obj = node.getObject())
        if (isPositiveAndBelow (index, obj->getParameters().size()))
            if (auto param = obj->getParameter (index, true))
            {
                const auto name = param->getName (64);
                if (name.isNotEmpty())
                    return name;
            }

    return index < 0 ? String ("—") : "Param " + String (index + 1);
}
} // namespace

MidiMappingsView::MidiMappingsView()
{
    setName ("MidiMappings");

    addAndMakeVisible (table);
    table.setModel (this);
    table.getHeader().addColumn ("Device", ColDevice, 160);
    table.getHeader().addColumn ("Event", ColEvent, 90);
    table.getHeader().addColumn ("Ch", ColChannel, 44);
    table.getHeader().addColumn ("Node", ColNode, 160);
    table.getHeader().addColumn ("Parameter", ColParameter, 160);
    table.setHeaderHeight (22);

    addAndMakeVisible (deleteButton);
    deleteButton.addListener (this);

    setEscapeTriggersClose (true);
}

MidiMappingsView::~MidiMappingsView()
{
    deleteButton.removeListener (this);
    table.setModel (nullptr);
}

void MidiMappingsView::resized()
{
    auto r = getLocalBounds().reduced (4);
    auto top = r.removeFromTop (26);
    deleteButton.setBounds (top.removeFromLeft (80));
    r.removeFromTop (4);
    table.setBounds (r);
}

void MidiMappingsView::stabilizeContent()
{
    table.updateContent();
    table.repaint();
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
            text = deviceName (mapping.getDevice());
            break;
        case ColEvent:
            text = (mapping.isNoteEvent() ? "Note " : "CC ") + String (mapping.getEventId());
            break;
        case ColChannel:
            text = mapping.getMidiChannel() == 0 ? "omni" : String (mapping.getMidiChannel());
            break;
        case ColNode: {
            auto node = session->findNodeById (mapping.getNodeUuid());
            text = node.isValid() ? node.getDisplayName() : String ("(missing)");
            break;
        }
        case ColParameter: {
            auto node = session->findNodeById (mapping.getNodeUuid());
            text = parameterName (node, mapping.getParameterIndex());
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

    if (b == &deleteButton)
    {
        const int row = table.getSelectedRow();
        if (auto session = ViewHelpers::getSession (this))
            if (isPositiveAndBelow (row, session->getNumMidiMappings()))
                maps->remove (session->getMidiMapping (row));
        stabilizeContent();
    }
}

} // namespace element

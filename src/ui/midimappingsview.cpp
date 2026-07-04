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

/** True if a node with the given uuid lives anywhere under this graph. */
bool graphContainsNode (const Node& graph, const juce::Uuid& target)
{
    for (int i = 0; i < graph.getNumNodes(); ++i)
    {
        const auto child = graph.getNode (i);
        if (child.getUuid() == target)
            return true;
        if (child.data().getNumChildren() > 0 && graphContainsNode (child, target))
            return true;
    }
    return false;
}

/** True if the node belongs to the session graph identified by graphUuid. */
bool nodeInGraph (SessionPtr session, const String& graphUuid, const juce::Uuid& node)
{
    for (int i = 0; i < session->getNumGraphs(); ++i)
    {
        const auto graph = session->getGraph (i);
        if (graph.getUuid().toString() == graphUuid)
            return graphContainsNode (graph, node);
    }
    return false;
}
} // namespace

//=============================================================================
/** Editable settings for a single MIDI mapping. Every editor is bound to the
    mapping's ValueTree, so edits persist immediately; a single tree listener
    pushes them live to the engine and rebuilds the list when the event type or
    target node changes (which alters the available parameters). */
class MidiMappingProperties : public juce::PropertyPanel,
                              private juce::ValueTree::Listener
{
public:
    MidiMappingProperties() = default;
    ~MidiMappingProperties() override { setMapping (MidiMapping(), nullptr); }

    /** Called after any edit so the owner can rebuild engine bindings + repaint. */
    std::function<void()> onEdited;

    /** Share the owner's list-filter values so the Filter section drives them.
        Value assignment shares the underlying source, so edits propagate back. */
    void setFilterValues (const juce::Value& device, const juce::Value& node, const juce::Value& graph)
    {
        filterDevice.referTo (device);
        filterNode.referTo (node);
        filterGraph.referTo (graph);
    }

    void setMapping (const MidiMapping& newMapping, SessionPtr newSession)
    {
        if (mapping.isValid())
            mapping.data().removeListener (this);

        mapping = newMapping;
        session = newSession;

        if (mapping.isValid())
            mapping.data().addListener (this);

        rebuild();
    }

private:
    MidiMapping mapping;
    SessionPtr session;
    juce::Value filterDevice, filterNode, filterGraph;

    void collectNodes (juce::StringArray& names, juce::Array<juce::var>& uuids, const Node& node)
    {
        for (int i = 0; i < node.getNumNodes(); ++i)
        {
            const auto child = node.getNode (i);
            if (child.getObject() != nullptr)
            {
                names.add (child.getDisplayName());
                uuids.add (child.getUuid().toString());
            }
            if (child.data().getNumChildren() > 0)
                collectNodes (names, uuids, child);
        }
    }

    void rebuild()
    {
        clear();
        if (session == nullptr)
            return;

        addFilterSection();

        if (! mapping.isValid())
            return;

        juce::Array<PropertyComponent*> comps;

        comps.add (new TextPropertyComponent (
            mapping.getPropertyAsValue (tags::name), TRANS ("Name"), 120, false));

        // Device: "(Any Device)" plus every connected input.
        {
            StringArray choices { TRANS ("Any Device") };
            Array<var> values { var (String()) };
            for (const auto& info : MidiInput::getAvailableDevices())
            {
                choices.add (info.name);
                values.add (info.identifier);
            }
            comps.add (new ChoicePropertyComponent (
                mapping.getPropertyAsValue (tags::device), TRANS ("Device"), choices, values));
        }

        comps.add (new ChoicePropertyComponent (
            mapping.getPropertyAsValue (tags::eventType), TRANS ("Event"), { "MIDI CC", "Note" }, { var ("controller"), var ("note") }));

        comps.add (new SliderPropertyComponent (
            mapping.getPropertyAsValue (tags::eventId),
            mapping.isNoteEvent() ? TRANS ("Note") : TRANS ("CC Number"),
            0.0,
            127.0,
            1.0));

        // Channel: Omni (0) or 1-16.
        {
            StringArray choices { TRANS ("Omni") };
            Array<var> values { var (0) };
            for (int i = 1; i <= 16; ++i)
            {
                choices.add (String (i));
                values.add (var (i));
            }
            comps.add (new ChoicePropertyComponent (
                mapping.getPropertyAsValue (tags::midiChannel), TRANS ("Channel"), choices, values));
        }

        // Latch is meaningful for notes only.
        if (mapping.isNoteEvent())
            comps.add (new BooleanPropertyComponent (
                mapping.getPropertyAsValue (tags::toggle), TRANS ("Latch"), TRANS ("Toggle on each note-on")));

        // Target node.
        {
            StringArray names;
            Array<var> uuids;
            for (int i = 0; i < session->getNumGraphs(); ++i)
                collectNodes (names, uuids, session->getGraph (i));
            comps.add (new ChoicePropertyComponent (
                mapping.getPropertyAsValue (tags::node), TRANS ("Node"), names, uuids));
        }

        // Parameters of the current target node (regular + special).
        {
            StringArray names;
            Array<var> indices;
            const auto node = session->findNodeById (mapping.getNodeUuid());
            if (auto* obj = node.getObject())
            {
                const auto& params = obj->getParameters();
                for (int i = 0; i < params.size(); ++i)
                {
                    auto name = params[i]->getName (64);
                    names.add (name.isNotEmpty() ? name : "Param " + String (i + 1));
                    indices.add (var (i));
                }
            }
            for (int special = Processor::EnabledParameter; special >= Processor::SpecialParameterBegin; --special)
            {
                names.add (Processor::getSpecialParameterName (special));
                indices.add (var (special));
            }
            comps.add (new ChoicePropertyComponent (
                mapping.getPropertyAsValue (tags::parameter), TRANS ("Parameter"), names, indices));
        }

        addSection (TRANS ("Mapping"), comps);
    }

    /** Filter controls for the whole list. Bound to the owner's shared values so
        picking a choice re-filters the table. Always shown, so a filter that hides
        every row can still be cleared. */
    void addFilterSection()
    {
        juce::Array<PropertyComponent*> comps;

        {
            StringArray choices { TRANS ("All Devices") };
            Array<var> values { var (String()) };
            for (const auto& info : MidiInput::getAvailableDevices())
            {
                choices.add (info.name);
                values.add (info.identifier);
            }
            comps.add (new ChoicePropertyComponent (filterDevice, TRANS ("Device"), choices, values));
        }

        {
            StringArray choices { TRANS ("All Graphs") };
            Array<var> values { var (String()) };
            for (int i = 0; i < session->getNumGraphs(); ++i)
            {
                const auto graph = session->getGraph (i);
                choices.add (graph.getDisplayName());
                values.add (graph.getUuid().toString());
            }
            comps.add (new ChoicePropertyComponent (filterGraph, TRANS ("Graph"), choices, values));
        }

        {
            StringArray choices { TRANS ("All Nodes") };
            Array<var> values { var (String()) };
            for (int i = 0; i < session->getNumGraphs(); ++i)
                collectNodes (choices, values, session->getGraph (i));
            comps.add (new ChoicePropertyComponent (filterNode, TRANS ("Node"), choices, values));
        }

        addSection (TRANS ("Filter"), comps);
    }

    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier& property) override
    {
        if (onEdited)
            onEdited();

        // Event type flips the CC/Note label and Latch row; node changes the
        // parameter list. Rebuild async so we never delete the editor mid-callback.
        if (property == tags::eventType || property == tags::node)
        {
            juce::Component::SafePointer<MidiMappingProperties> self (this);
            juce::MessageManager::callAsync ([self]() mutable {
                if (self != nullptr)
                    self->rebuild();
            });
        }
    }
};

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

    filterDevice.setValue (String());
    filterNode.setValue (String());
    filterGraph.setValue (String());
    filterDevice.addListener (this);
    filterNode.addListener (this);
    filterGraph.addListener (this);

    props = std::make_unique<MidiMappingProperties>();
    props->onEdited = [this]() { mappingEdited(); };
    props->setFilterValues (filterDevice, filterNode, filterGraph);
    addAndMakeVisible (*props);

    resizer = std::make_unique<StretchableLayoutResizerBar> (&layout, 1, true);
    addAndMakeVisible (*resizer);

    // table | resizer | properties
    layout.setItemLayout (0, 200.0, -1.0, -0.62);
    layout.setItemLayout (1, 4.0, 4.0, 4.0);
    layout.setItemLayout (2, 180.0, -1.0, -0.38);

    emptyLabel.setText (TRANS ("No MIDI mappings yet — press the map button and wiggle a control."),
                        dontSendNotification);
    emptyLabel.setJustificationType (Justification::centred);
    emptyLabel.setColour (Label::textColourId, Colours::grey);
    emptyLabel.setInterceptsMouseClicks (false, false);
    addChildComponent (emptyLabel);

    addAndMakeVisible (deleteButton);
    deleteButton.addListener (this);

    setEscapeTriggersClose (true);
}

MidiMappingsView::~MidiMappingsView()
{
    filterDevice.removeListener (this);
    filterNode.removeListener (this);
    filterGraph.removeListener (this);
    deleteButton.removeListener (this);
    table.setModel (nullptr);
    resizer = nullptr;
    props = nullptr;
}

void MidiMappingsView::resized()
{
    auto r = getLocalBounds().reduced (4);
    auto top = r.removeFromTop (26);
    deleteButton.setBounds (top.removeFromLeft (80));
    r.removeFromTop (4);

    Component* comps[] = { &table, resizer.get(), props.get() };
    layout.layOutComponents (comps, 3, r.getX(), r.getY(), r.getWidth(), r.getHeight(), false, true);

    emptyLabel.setBounds (table.getBounds().reduced (12));
}

void MidiMappingsView::stabilizeContent()
{
    updateRowOrder();
    table.updateContent();

    auto session = ViewHelpers::getSession (this);
    const int numRows = (int) rowOrder.size();
    const bool filtered = ! filterDevice.toString().isEmpty()
                          || ! filterNode.toString().isEmpty()
                          || ! filterGraph.toString().isEmpty();

    emptyLabel.setVisible (numRows == 0);
    if (numRows == 0)
        emptyLabel.setText (filtered ? TRANS ("No mappings match the current filter.")
                                     : TRANS ("No MIDI mappings yet — press the map button and wiggle a control."),
                            dontSendNotification);

    if (numRows > 0)
    {
        // Keep a row selected so the properties panel is always populated.
        if (! isPositiveAndBelow (table.getSelectedRow(), numRows))
            table.selectRow (0);
    }
    else
    {
        // No selectable row, but keep the Filter section available.
        props->setMapping (MidiMapping(), session);
    }

    table.repaint();
}

int MidiMappingsView::mappedRow (int row) const
{
    if (isPositiveAndBelow (row, (int) rowOrder.size()))
        return rowOrder[(size_t) row];
    return -1;
}

void MidiMappingsView::updateRowOrder()
{
    rowOrder.clear();

    auto session = ViewHelpers::getSession (this);
    if (session == nullptr)
        return;

    const int numMappings = session->getNumMidiMappings();
    for (int i = 0; i < numMappings; ++i)
        rowOrder.push_back (i);

    // Apply list filters (empty value = show all).
    {
        const String fDevice = filterDevice.toString();
        const String fNode = filterNode.toString();
        const String fGraph = filterGraph.toString();

        if (fDevice.isNotEmpty() || fNode.isNotEmpty() || fGraph.isNotEmpty())
        {
            std::vector<int> kept;
            for (const int idx : rowOrder)
            {
                const auto m = session->getMidiMapping (idx);
                if (fDevice.isNotEmpty() && m.getDevice() != fDevice)
                    continue;
                if (fNode.isNotEmpty() && m.getNodeUuid().toString() != fNode)
                    continue;
                if (fGraph.isNotEmpty() && ! nodeInGraph (session, fGraph, m.getNodeUuid()))
                    continue;
                kept.push_back (idx);
            }
            rowOrder.swap (kept);
        }
    }

    const int col = table.getHeader().getSortColumnId();
    if (col == 0)
        return;
    const bool forwards = table.getHeader().isSortedForwards();

    auto compare = [session, col] (int a, int b) -> int {
        auto ma = session->getMidiMapping (a);
        auto mb = session->getMidiMapping (b);
        switch (col)
        {
            case ColDevice:
                return deviceName (ma.getDevice()).compareIgnoreCase (deviceName (mb.getDevice()));
            case ColEvent:
                if (ma.getEventType() != mb.getEventType())
                    return ma.getEventType().compare (mb.getEventType());
                return ma.getEventId() - mb.getEventId();
            case ColChannel:
                return ma.getMidiChannel() - mb.getMidiChannel();
            case ColNode:
                return session->findNodeById (ma.getNodeUuid()).getDisplayName().compareIgnoreCase (session->findNodeById (mb.getNodeUuid()).getDisplayName());
            case ColParameter: {
                auto na = session->findNodeById (ma.getNodeUuid());
                auto nb = session->findNodeById (mb.getNodeUuid());
                return parameterName (na, ma.getParameterIndex())
                    .compareIgnoreCase (parameterName (nb, mb.getParameterIndex()));
            }
            default:
                return 0;
        }
    };

    std::stable_sort (rowOrder.begin(), rowOrder.end(), [&] (int a, int b) {
        const int c = compare (a, b);
        return forwards ? c < 0 : c > 0;
    });
}

void MidiMappingsView::sortOrderChanged (int, bool)
{
    updateRowOrder();
    table.repaint();
}

void MidiMappingsView::selectedRowsChanged (int row)
{
    auto session = ViewHelpers::getSession (this);
    const int index = mappedRow (row);
    if (session != nullptr && isPositiveAndBelow (index, session->getNumMidiMappings()))
        props->setMapping (session->getMidiMapping (index), session);
    else
        props->setMapping (MidiMapping(), nullptr);
}

void MidiMappingsView::mappingEdited()
{
    if (auto* cc = ViewHelpers::findContentComponent (this))
        if (auto* maps = cc->services().find<MappingService>())
            maps->refresh();
    updateRowOrder();
    table.repaint();
}

void MidiMappingsView::valueChanged (Value&)
{
    // A filter dropdown changed. Re-filter async so we never rebuild the panel
    // (which owns the dropdown that fired this) from inside its own callback.
    // Drop the selection first so the panel re-homes to the first visible match.
    Component::SafePointer<MidiMappingsView> self (this);
    MessageManager::callAsync ([self]() mutable {
        if (self != nullptr)
        {
            self->table.deselectAllRows();
            self->stabilizeContent();
        }
    });
}

int MidiMappingsView::getNumRows()
{
    return (int) rowOrder.size();
}

void MidiMappingsView::paintRowBackground (Graphics& g, int, int, int, bool selected)
{
    if (selected)
        g.fillAll (Colours::darkgrey.withAlpha (0.4f));
}

void MidiMappingsView::paintCell (Graphics& g, int row, int columnId, int w, int h, bool)
{
    auto session = ViewHelpers::getSession (this);
    const int index = mappedRow (row);
    if (session == nullptr || ! isPositiveAndBelow (index, session->getNumMidiMappings()))
        return;

    auto mapping = session->getMidiMapping (index);
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

void MidiMappingsView::removeMapping (int displayRow)
{
    auto* cc = ViewHelpers::findContentComponent (this);
    if (cc == nullptr)
        return;
    auto* maps = cc->services().find<MappingService>();
    if (maps == nullptr)
        return;

    const int index = mappedRow (displayRow);
    if (auto session = ViewHelpers::getSession (this))
        if (isPositiveAndBelow (index, session->getNumMidiMappings()))
            maps->remove (session->getMidiMapping (index));

    props->setMapping (MidiMapping(), nullptr);
    table.deselectAllRows();
    stabilizeContent();
}

void MidiMappingsView::buttonClicked (Button* b)
{
    if (b == &deleteButton)
        removeMapping (table.getSelectedRow());
}

void MidiMappingsView::deleteKeyPressed (int lastRowSelected)
{
    removeMapping (lastRowSelected);
}

void MidiMappingsView::cellClicked (int rowNumber, int, const MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        return;

    table.selectRow (rowNumber);

    PopupMenu menu;
    menu.addItem (1, TRANS ("Delete Mapping"));

    Component::SafePointer<MidiMappingsView> self (this);
    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&table),
                        [self, rowNumber] (int result) mutable {
                            if (self != nullptr && result == 1)
                                self->removeMapping (rowNumber);
                        });
}

} // namespace element

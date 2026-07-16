// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/node.hpp>

#include "ui/midimultichannelproperty.hpp"
#include "ui/nodeproperties.hpp"
#include "ui/nodemidiprogramcomponent.hpp"
#include "utils.hpp"

#ifndef EL_PROGRAM_NAME_PLACEHOLDER
#define EL_PROGRAM_NAME_PLACEHOLDER "Name..."
#endif

namespace element {

namespace detail {
// clang-format off
inline static bool showNodeDelayComp (const Node& node)
{
    if (node.isIONode() ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_MONITOR) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_PROGRAM_MAP) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_INPUT_DEVICE) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_SET_LIST) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_AUDIO_FILE_PLAYER) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_AUDIO_ROUTER) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_CHANNEL_MAP) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_CHANNEL_SPLITTER) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_CHANNELIZE) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_OUTPUT_DEVICE) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_ROUTER))
    {
        return false;
    }
    return true;
}

inline static bool showNodeMidiPrograms (const Node& node)
{
    if (node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_MONITOR) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_INPUT_DEVICE) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_OUTPUT_DEVICE))
    {
        return false;
    }
    return true;
}

inline static bool showMidiFilters (const Node& node)
{
    if (node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_MONITOR) || 
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_PROGRAM_MAP) || 
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_SET_LIST) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_INPUT_DEVICE) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_AUDIO_FILE_PLAYER) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_AUDIO_ROUTER) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_CHANNEL_MAP) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_CHANNEL_SPLITTER) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_CHANNELIZE) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_OUTPUT_DEVICE) ||
        node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_MIDI_ROUTER))
    {
        return false;
    }
    return true;
}
// clang-format on
} // namespace detail

class NodeMidiProgramPropertyComponent : public PropertyComponent
{
public:
    NodeMidiProgramPropertyComponent (const Node& n, const String& propertyName)
        : PropertyComponent (propertyName),
          node (n)
    {
        setPreferredHeight (40);

        addAndMakeVisible (program);

        program.name.onTextChange = [this]() {
            if (program.name.getText().isEmpty())
                program.name.setText (EL_PROGRAM_NAME_PLACEHOLDER, dontSendNotification);
            auto theText = program.name.getText();
            if (theText == EL_PROGRAM_NAME_PLACEHOLDER)
                theText = "";

            const auto programNumber = roundToInt (program.slider.getValue()) - 1;
            node.setMidiProgramName (programNumber, theText);
            updateMidiProgram();
        };

        program.slider.textFromValueFunction = [this] (double value) -> String {
            if (! node.areMidiProgramsEnabled())
                return "Off";
            return String (roundToInt (value));
        };
        program.slider.valueFromTextFunction = [] (const String& text) -> double {
            return text.getDoubleValue();
        };

        program.slider.onValueChange = [this]() {
            const auto newProgram = roundToInt (program.slider.getValue()) - 1;
            node.setMidiProgram (newProgram);
            updateMidiProgram();
        };

        program.slider.updateText();

        program.trashButton.setTooltip ("Delete MIDI program");
        program.trashButton.onClick = [this]() {
            if (ProcessorPtr ptr = node.getObject())
            {
                if (! ptr->areMidiProgramsEnabled())
                    return;
                ptr->removeMidiProgram (ptr->getMidiProgram(),
                                        ptr->useGlobalMidiPrograms());
                notifyProgramsChanged (ptr);
            }
        };

        program.saveButton.setTooltip ("Save MIDI program");
        program.saveButton.onClick = [this]() {
            if (ProcessorPtr ptr = node.getObject())
            {
                if (node.useGlobalMidiPrograms())
                {
                    if (isPositiveAndBelow (ptr->getMidiProgram(), 128))
                    {
                        node.savePluginState();
                        node.writeToFile (ptr->getMidiProgramFile());
                    }
                }
                else
                {
                    ptr->saveMidiProgram();
                }
                notifyProgramsChanged (ptr);
            }
        };

        program.loadButton.setTooltip ("Reload saved MIDI program");
        program.loadButton.onClick = [this]() {
            if (ProcessorPtr ptr = node.getObject())
            {
                if (isPositiveAndBelow (ptr->getMidiProgram(), 128))
                {
                    ptr->reloadMidiProgram();
                    stabilizeContent();
                }
            }
        };

        program.globalButton.onClick = [this]() {
            node.setUseGlobalMidiPrograms (program.globalButton.getToggleState());
            updateMidiProgram();
        };

        program.powerButton.onClick = [this]() {
            node.setMidiProgramsEnabled (program.powerButton.getToggleState());
            updateMidiProgram();
        };
    }

    void refresh() override
    {
        updateMidiProgram();
    }

private:
    Node node;
    NodeMidiProgramComponent program;

    /** Refreshes the properties panel (including the Programs table) after the
        set of saved programs changes. Deferred so we don't rebuild the panel —
        and destroy this component — from within a button callback. */
    static void notifyProgramsChanged (ProcessorPtr ptr)
    {
        MessageManager::callAsync ([ptr]() { ptr->midiProgramChanged(); });
    }

    void updateMidiProgram()
    {
        if (node.isGraph())
        {
            setEnabled (false);
            return;
        }

        setEnabled (true);
        const bool enabled = node.areMidiProgramsEnabled();
        String programName;
        if (ProcessorPtr object = node.getObject())
        {
            const bool global = object->useGlobalMidiPrograms();
            // use the object because there isn't a notifaction directly back to node model
            // in all cases
            const auto programNumber = object->getMidiProgram();
            program.slider.setValue (1 + object->getMidiProgram(), dontSendNotification);
            if (isPositiveAndNotGreaterThan (roundToInt (program.slider.getValue()), 128))
            {
                programName = node.getMidiProgramName (programNumber);
                program.name.setEnabled (global ? false : enabled);
                program.loadButton.setEnabled (enabled);
                program.saveButton.setEnabled (enabled);
                program.trashButton.setEnabled (enabled);
                program.powerButton.setToggleState (enabled, dontSendNotification);
            }
            else
            {
                program.name.setEnabled (false);
                program.loadButton.setEnabled (false);
                program.saveButton.setEnabled (false);
                program.trashButton.setEnabled (false);
                program.powerButton.setToggleState (false, dontSendNotification);
            }
        }

        program.name.setText (programName.isNotEmpty() ? programName : EL_PROGRAM_NAME_PLACEHOLDER, dontSendNotification);
        program.powerButton.setToggleState (node.areMidiProgramsEnabled(), dontSendNotification);
        program.globalButton.setToggleState (node.useGlobalMidiPrograms(), dontSendNotification);
        program.globalButton.setEnabled (enabled);
        program.slider.updateText();
        program.slider.setEnabled (enabled);
    }

    void stabilizeContent() {}
};

class NodeMidiChannelsPropertyComponent : public MidiMultiChannelPropertyComponent
{
public:
    NodeMidiChannelsPropertyComponent (const Node& n)
        : node (n)
    {
        setChannels (node.getMidiChannels().get());
        getChannelsValue().referTo (node.getPropertyAsValue (tags::midiChannels, false));
        changed.connect (std::bind (&NodeMidiChannelsPropertyComponent::onChannelsChanged, this));
    }

    ~NodeMidiChannelsPropertyComponent()
    {
        changed.disconnect_all_slots();
    }

    void onChannelsChanged()
    {
        // noop
    }

    Node node;
};

class MidiNotePropertyComponent : public SliderPropertyComponent
{
public:
    MidiNotePropertyComponent (const Value& value, const String& name)
        : SliderPropertyComponent (value, name, 0.0, 127.0, 1.0, 1.0, false)
    {
        slider.textFromValueFunction = Util::noteValueToString;
        slider.valueFromTextFunction = [this] (const String& text) -> double {
            const int note = Util::noteValueFromString (text);
            return note >= 0 ? (double) note : slider.getValue();
        };

        slider.updateText();
    }
};

class MillisecondSliderPropertyComponent : public SliderPropertyComponent
{
public:
    MillisecondSliderPropertyComponent (const Value& value, const String& name)
        : SliderPropertyComponent (value, name, -1000.0, 1000.0, 0.1, 1.0, false)
    {
        slider.textFromValueFunction = [] (double value) {
            String str (value, 1);
            str << " ms";
            return str;
        };

        slider.valueFromTextFunction = [] (const String& text) -> double {
            return text.replace ("ms", "", false).trim().getFloatValue();
        };

        slider.updateText();
    }
};

//==============================================================================
/** Lists the saved MIDI programs for a node in a selectable table.

    Single click selects a row, double-click loads that program, the name is
    renamed inline, and each row can be deleted. Follows the node's current
    mode: local in-memory programs or global program files on disk.
*/
class NodeMidiProgramsListPropertyComponent : public PropertyComponent,
                                              public ListBoxModel
{
public:
    NodeMidiProgramsListPropertyComponent (const Node& n)
        : PropertyComponent ("Saved Programs"),
          node (n)
    {
        fetchPrograms();

        list.setModel (this);
        list.setRowHeight (rowHeight);
        list.getViewport()->setScrollBarsShown (true, false);
        list.setVisible (! programs.isEmpty()); // let the empty hint show through
        addChildComponent (list);

        // Show up to maxVisibleRows rows before scrolling.
        const int rows = jlimit (1, maxVisibleRows, jmax (1, programs.size()));
        setPreferredHeight (rows * rowHeight + 2);

        selectCurrentProgramRow();
    }

    /** Draw the full-width table with no property label. PropertyComponent
        normally reserves its left half for the name and squeezes the editor
        into the right half; we don't want that here. */
    void paint (Graphics& g) override
    {
        if (programs.isEmpty())
        {
            g.setColour (Colors::textColor.withAlpha (0.5f));
            g.setFont (Font (FontOptions (12.f)));
            g.drawText ("No saved programs", getLocalBounds().reduced (6, 0), Justification::centredLeft);
        }
    }

    void resized() override
    {
        list.setBounds (getLocalBounds());
    }

    void refresh() override
    {
        fetchPrograms();
        list.setVisible (! programs.isEmpty());
        list.updateContent();
        selectCurrentProgramRow();
        list.repaint();
        repaint();
    }

    //==========================================================================
    int getNumRows() override { return programs.size(); }

    void paintListBoxItem (int, Graphics&, int, int, bool) override
    {
        // rows are drawn by their ProgramRow component
    }

    Component* refreshComponentForRow (int rowNumber, bool, Component* existing) override
    {
        std::unique_ptr<ProgramRow> row (dynamic_cast<ProgramRow*> (existing));
        if (! isPositiveAndBelow (rowNumber, programs.size()))
            return nullptr;

        if (row == nullptr)
            row = std::make_unique<ProgramRow> (*this);

        row->update (rowNumber, programs.getReference (rowNumber), isGlobal);
        return row.release();
    }

    void listBoxItemDoubleClicked (int rowNumber, const MouseEvent&) override
    {
        loadRow (rowNumber);
    }

private:
    Node node;
    ListBox list;
    Array<Processor::MidiProgramInfo> programs;
    bool isGlobal = false;

    static constexpr int rowHeight = 22;
    static constexpr int maxVisibleRows = 6;

    void fetchPrograms()
    {
        programs.clearQuick();
        if (ProcessorPtr ptr = node.getObject())
        {
            isGlobal = ptr->useGlobalMidiPrograms();
            programs = ptr->getMidiPrograms();
        }
    }

    void selectRow (int rowNumber)
    {
        list.selectRow (rowNumber);
    }

    /** Loads the given row's program, mirroring the slider's load behaviour. */
    void loadRow (int rowNumber)
    {
        if (! isPositiveAndBelow (rowNumber, programs.size()))
            return;
        if (ProcessorPtr ptr = node.getObject())
        {
            const int program = programs.getReference (rowNumber).program;
            if (program == ptr->getMidiProgram())
                return; // already loaded — don't discard live edits by reloading
            node.setMidiProgram (program);
            ptr->reloadMidiProgram(); // fires midiProgramChanged -> panel refreshes
        }
    }

    /** Highlights the row matching the node's currently-loaded MIDI program.
        The current program can change from a table click, the slider selector,
        or a Program Change message received externally. All three update the
        Processor's program (the single source of truth here — the Node model
        property is not touched by externally-received program changes), and all
        funnel through midiProgramChanged, which rebuilds this component. */
    void selectCurrentProgramRow()
    {
        ProcessorPtr ptr = node.getObject();
        const int current = ptr != nullptr ? ptr->getMidiProgram() : -1;
        for (int i = 0; i < programs.size(); ++i)
        {
            if (programs.getReference (i).program == current)
            {
                list.selectRow (i);
                return;
            }
        }
        list.deselectAllRows();
    }

    void renameRow (int rowNumber, const String& newName)
    {
        if (isGlobal || ! isPositiveAndBelow (rowNumber, programs.size()))
            return;
        const auto program = programs.getReference (rowNumber).program;
        node.setMidiProgramName (program, newName);
        programs.getReference (rowNumber).name = newName;
    }

    /** Renumbers a row's saved program. @p newProgram is zero-based (0-127). */
    void changeRowNumber (int rowNumber, int newProgram)
    {
        if (isGlobal || ! isPositiveAndBelow (rowNumber, programs.size()))
            return;
        const auto oldProgram = programs.getReference (rowNumber).program;
        if (newProgram == oldProgram)
            return;
        if (ProcessorPtr ptr = node.getObject())
        {
            if (! ptr->changeMidiProgramNumber (oldProgram, newProgram))
                return;
            // Refresh reorders/rebuilds rows, deleting the row we're called
            // from — defer so we don't tear it down mid-callback.
            Component::SafePointer<NodeMidiProgramsListPropertyComponent> safe (this);
            MessageManager::callAsync ([safe]() {
                if (safe != nullptr)
                    safe->refresh();
            });
        }
    }

    void deleteRow (int rowNumber)
    {
        if (! isPositiveAndBelow (rowNumber, programs.size()))
            return;
        if (ProcessorPtr ptr = node.getObject())
        {
            ptr->removeMidiProgram (programs.getReference (rowNumber).program,
                                    ptr->useGlobalMidiPrograms());
            refresh();
        }
    }

    //==========================================================================
    /** A single program row: number, editable name and a delete button. */
    class ProgramRow : public Component
    {
    public:
        ProgramRow (NodeMidiProgramsListPropertyComponent& o)
            : owner (o)
        {
            addAndMakeVisible (number);
            number.setFont (Font (FontOptions (12.f)));
            number.setColour (Label::textColourId, Colors::textColor.withAlpha (0.6f));
            number.setJustificationType (Justification::centredLeft);
            number.setEditable (false, true, false); // edit on double-click
            // Forward the label's clicks so single-clicking selects the row.
            number.addMouseListener (this, false);

            number.onEditorShow = [this]() {
                if (auto* ed = number.getCurrentTextEditor())
                {
                    ed->setInputRestrictions (3, "0123456789");
                    ed->setText (String (programNumber + 1), false);
                    ed->selectAll();
                }
            };
            number.onTextChange = [this]() {
                const int entered = number.getText().getIntValue();
                if (entered >= 1 && entered <= 128 && (entered - 1) != programNumber)
                    owner.changeRowNumber (row, entered - 1);
                // Revert the display; a valid change refreshes the list, an
                // invalid one just restores the current number.
                number.setText (String (programNumber + 1), dontSendNotification);
            };

            addAndMakeVisible (name);
            name.setFont (Font (FontOptions (12.f)));
            name.setEditable (false, true, false);
            // Forward the label's clicks so single-clicking the name selects the row.
            name.addMouseListener (this, false);

            // Edit the real (possibly empty) name, showing the placeholder as ghost text.
            name.onEditorShow = [this]() {
                if (auto* ed = name.getCurrentTextEditor())
                {
                    ed->setText (realName, false);
                    ed->setTextToShowWhenEmpty (placeholder, Colors::textColor.withAlpha (0.5f));
                    ed->selectAll();
                }
            };
            name.onTextChange = [this]() {
                realName = name.getText() == placeholder ? String() : name.getText();
                owner.renameRow (row, realName);
                updateNameDisplay();
            };

            addAndMakeVisible (trashButton);
            trashButton.setTooltip ("Delete MIDI program");
            trashButton.setIcon (Icon (getIcons().farTrashAlt, Colors::textColor));
            trashButton.onClick = [this]() { owner.deleteRow (row); };
        }

        void update (int newRow, const Processor::MidiProgramInfo& info, bool global)
        {
            row = newRow;
            programNumber = info.program;
            number.setText (String (info.program + 1), dontSendNotification);
            number.setEditable (false, ! global, false);
            realName = info.name;
            placeholder = "Program " + String (info.program + 1);
            name.setEditable (false, ! global, false);
            updateNameDisplay();
            resized();
            repaint();
        }

        /** Show the name, or the dimmed "Program N" placeholder when blank. */
        void updateNameDisplay()
        {
            const bool blank = realName.isEmpty();
            name.setText (blank ? placeholder : realName, dontSendNotification);
            name.setColour (Label::textColourId,
                            blank ? Colors::textColor.withAlpha (0.5f) : Colors::textColor);
        }

        void paint (Graphics& g) override
        {
            // Query live selection so the highlight follows ListBox::selectRow.
            if (owner.list.isRowSelected (row))
                g.fillAll (Colors::widgetBackgroundColor.brighter (0.15f));
        }

        void resized() override
        {
            auto r = getLocalBounds();
            number.setBounds (r.removeFromLeft (numberWidth).withTrimmedLeft (4));
            trashButton.setBounds (r.removeFromRight (20).reduced (0, 1));
            r.removeFromRight (2);
            name.setBounds (r);
        }

        void mouseDown (const MouseEvent&) override
        {
            owner.selectRow (row); // instant highlight before the async reload completes
            owner.loadRow (row);
        }
        void mouseDoubleClick (const MouseEvent& e) override
        {
            // Double-clicking the number or name edits it (handled by the
            // Label); only double-clicks elsewhere on the row load the program.
            if (e.eventComponent == &name || e.eventComponent == &number)
                return;
            owner.loadRow (row);
        }

    private:
        static constexpr int numberWidth = 30;
        NodeMidiProgramsListPropertyComponent& owner;
        int row = -1;
        int programNumber = -1;
        String realName;
        String placeholder;
        Label number;
        Label name;
        IconButton trashButton;
    };
};

//==============================================================================
NodeProperties::NodeProperties (const Node& n, int groups)
    : NodeProperties (n, groups & General, groups & Midi, groups & Programs) {}

NodeProperties::NodeProperties (const Node& n, bool nodeProps, bool midiProps, bool programsProps)
{
    Node node = n;

    if (nodeProps)
    {
        add (new TextPropertyComponent (node.getPropertyAsValue (tags::name),
                                        "Name",
                                        100,
                                        false,
                                        true));
        if (detail::showNodeDelayComp (node))
            add (new MillisecondSliderPropertyComponent (
                node.getPropertyAsValue (tags::delayCompensation), "Delay comp."));
    }

    if (midiProps)
    {
        // MIDI Channel
        add (new NodeMidiChannelsPropertyComponent (node));

        if (detail::showMidiFilters (node))
        {
            // Key Start
            add (new MidiNotePropertyComponent (node.getPropertyAsValue (tags::keyStart, false), "Key Start"));

            // Key End
            add (new MidiNotePropertyComponent (node.getPropertyAsValue (tags::keyEnd, false), "Key End"));

            // Transpose
            add (new SliderPropertyComponent (node.getPropertyAsValue (tags::transpose, false), "Transpose", -24.0, 24.0, 1.0));
        }
    }

    if (programsProps && detail::showNodeMidiPrograms (node))
    {
        // MIDI Program selector (moved from the MIDI section) sits above the table.
        add (new NodeMidiProgramPropertyComponent (node, "MIDI Program"));

        // Table of saved MIDI programs.
        add (new NodeMidiProgramsListPropertyComponent (node));
    }
}

} // namespace element

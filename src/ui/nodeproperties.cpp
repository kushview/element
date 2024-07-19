// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

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
        slider.valueFromTextFunction = [] (const String& text) -> double {
            return 0.0;
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

NodeProperties::NodeProperties (const Node& n, int groups)
    : NodeProperties (n, groups & General, groups & Midi) {}

NodeProperties::NodeProperties (const Node& n, bool nodeProps, bool midiProps)
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

        if (detail::showNodeMidiPrograms (node))
        {
            // MIDI Program
            add (new NodeMidiProgramPropertyComponent (node, "MIDI Program"));
        }

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
}

} // namespace element

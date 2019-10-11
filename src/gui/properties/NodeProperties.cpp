
#include "gui/properties/MidiMultiChannelPropertyComponent.h"
#include "gui/properties/NodeProperties.h"
#include "gui/widgets/NodeMidiProgramComponent.h"
#include "session/Node.h"
#include "Utils.h"

#ifndef EL_PROGRAM_NAME_PLACEHOLDER
 #define EL_PROGRAM_NAME_PLACEHOLDER "Name..."
#endif

namespace Element {

class NodeMidiProgramPropertyComponent : public PropertyComponent
{
public:
    NodeMidiProgramPropertyComponent (const Node& n, const String& propertyName)
        : PropertyComponent (propertyName),
          node (n)
    {
        setPreferredHeight (40);

        addAndMakeVisible (program);

        program.name.onTextChange = [this]()
        {
            if (program.name.getText().isEmpty())
                program.name.setText (EL_PROGRAM_NAME_PLACEHOLDER, dontSendNotification);
            auto theText = program.name.getText();
            if (theText == EL_PROGRAM_NAME_PLACEHOLDER)
                theText = "";

            const auto programNumber = roundToInt (program.slider.getValue()) - 1;
            node.setMidiProgramName (programNumber, theText);
            updateMidiProgram();
        };

        program.slider.textFromValueFunction = [this](double value) -> String {
            if (! node.areMidiProgramsEnabled()) return "Off";
            return String (roundToInt (value));
        };
        program.slider.valueFromTextFunction = [this](const String& text) -> double {
            return text.getDoubleValue();
        };

        program.slider.onValueChange = [this]() {
            const auto newProgram = roundToInt (program.slider.getValue()) - 1;
            node.setMidiProgram (newProgram);
            updateMidiProgram();
        };

        program.slider.updateText();

        program.trashButton.setTooltip ("Delete MIDI program");
        program.trashButton.onClick = [this]()
        {
            if (GraphNodePtr ptr = node.getGraphNode())
            {
                if (! ptr->areMidiProgramsEnabled())
                    return;
                ptr->removeMidiProgram (ptr->getMidiProgram(),
                                        ptr->useGlobalMidiPrograms());
            }
        };

        program.saveButton.setTooltip ("Save MIDI program");
        program.saveButton.onClick = [this]()
        {
            if (GraphNodePtr ptr = node.getGraphNode())
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
        program.loadButton.onClick = [this]()
        {
            if (GraphNodePtr ptr = node.getGraphNode())
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
        const bool enabled = node.areMidiProgramsEnabled();
        String programName;
        if (GraphNodePtr object = node.getGraphNode())
        {
            // use the object because there isn't a notifaction directly back to node model
            // in all cases
            const auto programNumber = object->getMidiProgram();
            program.slider.setValue (1 + object->getMidiProgram(), dontSendNotification);
            if (isPositiveAndNotGreaterThan (roundToInt (program.slider.getValue()), 128))
            {
                programName = node.getMidiProgramName (programNumber);
                program.name.setEnabled (enabled);
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
        
        program.name.setText (programName.isNotEmpty() ? 
            programName : EL_PROGRAM_NAME_PLACEHOLDER, dontSendNotification);
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
        getChannelsValue().referTo (node.getPropertyAsValue (Tags::midiChannels, false));
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
        slider.valueFromTextFunction = [this](const String& text) -> double {
            return 0.0;
        };

        slider.updateText();
    }
};

NodeProperties::NodeProperties (const Node& n, bool nodeProps, bool midiProps)
{
    Node node = n;

    if (nodeProps)
    {
        add (new TextPropertyComponent (node.getPropertyAsValue (Tags::name), 
            "Name", 100, false, true));
    }

    if (midiProps)
    {
        // MIDI Channel
        add (new NodeMidiChannelsPropertyComponent (node));
        
       #if defined (EL_PRO) || defined (EL_SOLO)
        // MIDI Program
        add (new NodeMidiProgramPropertyComponent (node, "MIDI Program"));
       #endif

        // Key Start
        add (new MidiNotePropertyComponent (node.getPropertyAsValue (Tags::keyStart, false), "Key Start"));
        
        // Key End
        add (new MidiNotePropertyComponent (node.getPropertyAsValue (Tags::keyEnd, false), "Key End"));
        
        // Transpose
        add (new SliderPropertyComponent (node.getPropertyAsValue (Tags::transpose, false), "Transpose", -24.0, 24.0, 1.0));
    }
}

}

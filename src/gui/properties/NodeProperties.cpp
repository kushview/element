
#include "gui/properties/MidiMultiChannelPropertyComponent.h"
#include "gui/properties/NodeProperties.h"
#include "gui/widgets/NodeMidiProgramComponent.h"
#include "session/Node.h"

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
        program.name.setText ("Program name...", dontSendNotification);
        program.name.setFont (Font (12.f));
        program.name.setEditable (false, true, false);
        program.name.setTooltip ("MIDI Program name");
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

    }

private:
    Node node;
    NodeMidiProgramComponent program;
    void updateMidiProgram() {}
    void stabilizeContent() {}
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
        auto* mcp = new MidiMultiChannelPropertyComponent();
        mcp->getChannelsValue().referTo (
            node.getPropertyAsValue (Tags::midiChannels, false));
        add (mcp);

        // MIDI Program
        add (new NodeMidiProgramPropertyComponent (node, "MIDI Program"));

        // Key Start
        add (new SliderPropertyComponent (node.getPropertyAsValue (Tags::keyStart, false),
            "Key Start", 0.0, 127.0, 1.0));
        
        // Key End
        add (new SliderPropertyComponent (node.getPropertyAsValue (Tags::keyEnd, false),
            "Key End", 0.0, 127.0, 1.0));
        
        // Transpose
        add (new SliderPropertyComponent (node.getPropertyAsValue (Tags::transpose, false),
            "Transpose", -24.0, 24.0, 1.0));
    }
}

}

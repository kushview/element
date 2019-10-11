
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/LookAndFeel.h"
#include "gui/properties/NodeProperties.h"
#include "gui/views/NodeMidiContentView.h"
#include "gui/ViewHelpers.h"

#define EL_PROGRAM_NAME_PLACEHOLDER "Name..."
#define EL_NODE_MIDI_CONTENT_VIEW_PROPS 1

namespace Element {

NodeMidiContentView::NodeMidiContentView()
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
    setInterceptsMouseClicks (true, true);
    addAndMakeVisible (props);
    updateProperties();
}

NodeMidiContentView::~NodeMidiContentView()
{
    midiProgramChangedConnection.disconnect();
    selectedNodeConnection.disconnect();
    transposeLabel.onDoubleClicked = nullptr;
    keyLowLabel.onDoubleClicked = nullptr;
    keyHiLabel.onDoubleClicked = nullptr;
    midiChannelLabel.onDoubleClicked = nullptr;
    midiChannel.onChanged = nullptr;
}

void NodeMidiContentView::paint (Graphics& g)
{
    g.fillAll (Element::LookAndFeel::backgroundColor);
}

void NodeMidiContentView::resized()
{
    auto r (getLocalBounds().reduced (2));
    props.setBounds (r);
}

void NodeMidiContentView::stabilizeContent()
{
    auto *cc = ViewHelpers::findContentComponent(this);
    jassert(cc);
    auto& gui = *cc->getAppController().findChild<GuiController>();
    if (! selectedNodeConnection.connected())
        selectedNodeConnection = gui.nodeSelected.connect (std::bind (
            &NodeMidiContentView::stabilizeContent, this));

    midiProgramChangedConnection.disconnect();
    node = gui.getSelectedNode();
    nodeSync.setFrozen (true);
    nodeSync.setNode (node);

    if (node.isValid() && ! node.isIONode())
    {
        setEnabled (true);
        updateProperties();

        if (GraphNodePtr ptr = node.getGraphNode())
        {
            #if defined (EL_PRO) || defined (EL_SOLO)
            ptr->midiProgramChanged.connect (
                std::bind (&NodeMidiContentView::updateMidiProgram, this));
            #endif
        }
    }
    else
    {
        setEnabled (false);
    }

    nodeSync.setFrozen (false);
}

void NodeMidiContentView::updateProperties()
{
    #if EL_NODE_MIDI_CONTENT_VIEW_PROPS
    props.clear();
    props.addProperties (NodeProperties (node, false, true));
    resized();
    #endif
}

void NodeMidiContentView::updateMidiProgram()
{
    const bool enabled = node.areMidiProgramsEnabled();
    String programName;
    if (GraphNodePtr object = node.getGraphNode())
    {
        // use the object because there isn't a notifaction directly back to node model
        // in all cases
        const auto programNumber = object->getMidiProgram();
        midiProgram.slider.setValue (1 + object->getMidiProgram(), dontSendNotification);
        if (isPositiveAndNotGreaterThan (roundToInt (midiProgram.slider.getValue()), 128))
        {
            programName = node.getMidiProgramName (programNumber);
            midiProgram.name.setEnabled (enabled);
            midiProgram.loadButton.setEnabled (enabled);
            midiProgram.saveButton.setEnabled (enabled);
            midiProgram.trashButton.setEnabled (enabled);
            midiProgram.powerButton.setToggleState (enabled, dontSendNotification);
        }
        else
        {
            midiProgram.name.setEnabled (false);
            midiProgram.loadButton.setEnabled (false);
            midiProgram.saveButton.setEnabled (false);
            midiProgram.trashButton.setEnabled (false);
            midiProgram.powerButton.setToggleState (false, dontSendNotification);
        }
    }
    
    midiProgram.name.setText (programName.isNotEmpty() ? 
        programName : EL_PROGRAM_NAME_PLACEHOLDER, dontSendNotification);
    midiProgram.powerButton.setToggleState (node.areMidiProgramsEnabled(), dontSendNotification);
    midiProgram.globalButton.setToggleState (node.useGlobalMidiPrograms(), dontSendNotification);
    midiProgram.globalButton.setEnabled (enabled);
    midiProgram.slider.updateText();
    midiProgram.slider.setEnabled (enabled);
}

}

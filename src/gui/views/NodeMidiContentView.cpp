
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/LookAndFeel.h"
#include "gui/properties/NodeProperties.h"
#include "gui/views/NodeMidiContentView.h"
#include "gui/ViewHelpers.h"

#define EL_PROGRAM_NAME_PLACEHOLDER "Name..."
#define EL_NODE_MIDI_CONTENT_VIEW_PROPS 1

namespace Element {

    static String noteValueToString (double value)
    {
        return MidiMessage::getMidiNoteName (roundToInt (value), true, true, 3);
    }

    NodeMidiContentView::NodeMidiContentView()
    {
        const Font font (12.f);
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
        setInterceptsMouseClicks (true, true);

       #if EL_NODE_MIDI_CONTENT_VIEW_PROPS
        addAndMakeVisible (props);
       #endif
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
        keyLowSlider.removeListener (this);
        keyHiSlider.removeListener (this);
        transposeSlider.removeListener (this);
    }

    void NodeMidiContentView::paint (Graphics& g)
    {
        g.fillAll (Element::LookAndFeel::backgroundColor);
    }

    void NodeMidiContentView::resized()
    {
        auto r (getLocalBounds().reduced (2));

       #if EL_NODE_MIDI_CONTENT_VIEW_PROPS
        props.setBounds(r);
        return;
       #endif

        const auto estimatedH = 4 + (7 * 26) + 2 + 2;
        r = r.withHeight (jmax (estimatedH, r.getHeight()));

        r.removeFromTop (4);
        r.removeFromRight (4);
        
        layoutComponent (r, midiChannelLabel, midiChannel, 
                         midiChannel.getSuggestedHeight (r.getWidth()));
       #if defined (EL_PRO) || defined (EL_SOLO)
        r.removeFromTop (2);
       
        layoutComponent (r, midiProgramLabel, midiProgram, 40);
        r.removeFromTop (2);
       #endif
        layoutComponent (r, keyLowLabel, keyLowSlider);
        layoutComponent (r, keyHiLabel, keyHiSlider);
        layoutComponent (r, transposeLabel, transposeSlider);
    }

    void NodeMidiContentView::layoutComponent (Rectangle<int>& r, Label& l, Component& c,
                                           int preferedHeight)
    {
        static const int settingHeight = 20;
        static const int labelWidth = 64;
        static const int spacing = 6;
        auto r2 = r.removeFromTop (preferedHeight > 0 ? preferedHeight : settingHeight);
        l.setBounds (r2.removeFromLeft (labelWidth));
        c.setBounds (r2);
        r.removeFromTop (spacing);
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

    void NodeMidiContentView::sliderValueChanged (Slider* slider)
    {
        GraphNodePtr object = node.getGraphNode();
        if (object == nullptr || ! node.isValid())
            return;
        
        if (slider == &keyLowSlider)
        {
            auto keyRange (object->getKeyRange());
            keyRange.setStart (roundToInt (slider->getValue()));
            object->setKeyRange (keyRange);
        }
        else if (slider == &keyHiSlider)
        {
            auto keyRange (object->getKeyRange());
            keyRange.setEnd (roundToInt (slider->getValue()));
            object->setKeyRange (keyRange);
        }
        else if (slider == &transposeSlider)
        {
            object->setTransposeOffset (roundToInt (slider->getValue()));
        }
        
        ValueTree data (node.getValueTree());
        auto range (object->getKeyRange());
        data.setProperty (Tags::keyStart, range.getStart(), nullptr)
            .setProperty (Tags::keyEnd, range.getEnd(), nullptr)
            .setProperty (Tags::transpose, object->getTransposeOffset(), nullptr);

        updateSliders(); // in case graph node changes requested values
    }

    void NodeMidiContentView::updateProperties()
    {
       #if EL_NODE_MIDI_CONTENT_VIEW_PROPS
        props.clear();
        props.addProperties (NodeProperties (node, false, true));
        resized();
       #endif
    }

    void NodeMidiContentView::updateSliders()
    {
        if (GraphNodePtr object = node.getGraphNode())
        {
            const auto range (object->getKeyRange());
            keyLowSlider.setValue ((double) range.getStart(), dontSendNotification);
            keyHiSlider.setValue ((double) range.getEnd(), dontSendNotification);
            transposeSlider.setValue ((double) object->getTransposeOffset(), dontSendNotification);
        }
    }

    void NodeMidiContentView::updateMidiChannels()
    {
        if (GraphNodePtr object = node.getGraphNode())
        {
            BigInteger chans;
            {
                ScopedLock sl (object->getPropertyLock());
                chans = object->getMidiChannels().get();
            }

            midiChannel.setChannels (chans, false);
        }
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


#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/LookAndFeel.h"
#include "gui/views/NodeMidiContentView.h"
#include "gui/ViewHelpers.h"

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

        addAndMakeVisible (nameLabel);
        nameLabel.setText ("Name", dontSendNotification);
        nameLabel.setFont (font);
        addAndMakeVisible (nameEditor);
        
        addAndMakeVisible (transposeLabel);
        transposeLabel.setText ("Transpose", dontSendNotification);
        transposeLabel.setFont (font);
        addAndMakeVisible (transposeSlider);
        transposeSlider.setRange (-24, 24, 1);
        transposeSlider.setValue (0);
        transposeSlider.setSliderStyle (Slider::LinearHorizontal);
        transposeSlider.setTextBoxStyle (Slider::TextBoxRight, true, 40, 18);
        
        addAndMakeVisible (keyLowLabel);
        keyLowLabel.setText ("Key Start", dontSendNotification);
        keyLowLabel.setFont (font);
        addAndMakeVisible (keyLowSlider);
        keyLowSlider.textFromValueFunction = noteValueToString;
        keyLowSlider.setRange (0, 127, 1.0);
        keyLowSlider.setSliderStyle (Slider::LinearHorizontal);
        keyLowSlider.setTextBoxStyle (Slider::TextBoxRight, true, 40, 18);
        keyLowSlider.setTextBoxIsEditable (false);
        keyLowSlider.setValue (0);

        addAndMakeVisible (keyHiLabel);
        keyHiLabel.setText ("Key End", dontSendNotification);
        keyHiLabel.setFont (font);
        addAndMakeVisible (keyHiSlider);
        keyHiSlider.textFromValueFunction = noteValueToString;
        keyHiSlider.setRange (0, 127, 1.0);
        keyHiSlider.setSliderStyle (Slider::LinearHorizontal);
        keyHiSlider.setTextBoxStyle (Slider::TextBoxRight, true, 40, 18);
        keyHiSlider.setTextBoxIsEditable (false);
        keyHiSlider.setValue (127);

        addAndMakeVisible (midiProgramLabel);
        midiProgramLabel.setText ("MIDI Prog.", dontSendNotification);
        addAndMakeVisible (midiProgram);

        midiProgram.slider.textFromValueFunction = [this](double value) -> String {
            if (! node.areMidiProgramsEnabled()) return "Off";
            return String (roundToInt (value));
        };
        midiProgram.slider.valueFromTextFunction = [this](const String& text) -> double {
            return text.getDoubleValue();
        };

        midiProgram.slider.onValueChange = [this]() {
            const auto program = roundToInt (midiProgram.slider.getValue()) - 1;
            node.setMidiProgram (program);
            updateMidiProgram();
        };

        midiProgramLabel.onDoubleClicked = [this](const MouseEvent&) {
            midiProgram.slider.setValue (0.0);
        };
        midiProgram.slider.updateText();

        midiProgram.saveButton.setTooltip ("Save MIDI program");
        midiProgram.saveButton.onClick = [this]()
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

        midiProgram.loadButton.setTooltip ("Reload saved MIDI program");
        midiProgram.loadButton.onClick = [this]()
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

        midiProgram.globalButton.onClick = [this]() {
            node.setUseGlobalMidiPrograms (midiProgram.globalButton.getToggleState());
            updateMidiProgram();
        };
        midiProgram.powerButton.onClick = [this]() {
            node.setMidiProgramsEnabled (midiProgram.powerButton.getToggleState());
            updateMidiProgram();
        };

        addAndMakeVisible (midiChannelLabel);
        midiChannelLabel.setText ("MIDI Ch.", dontSendNotification);
        midiChannelLabel.setFont (font);
        
        
        addAndMakeVisible (midiChannel);

        transposeLabel.onDoubleClicked = [this](const MouseEvent&) {
            transposeSlider.setValue (0.0, sendNotificationAsync);
        };

        keyLowLabel.onDoubleClicked = [this](const MouseEvent&) {
            keyLowSlider.setValue (0.0, sendNotificationAsync);
        };

        keyHiLabel.onDoubleClicked = [this](const MouseEvent&) {
            keyHiSlider.setValue (127.0, sendNotificationAsync); 
        };

        midiChannelLabel.onDoubleClicked = [this](const MouseEvent&) {
            BigInteger chans;
            chans.setRange (0, 17, false);
            chans.setBit (0, true);
            midiChannel.setChannels (chans);
        };

        midiChannel.onChanged = [this]()  {
            if (auto* o = node.getGraphNode())
            {
                o->setMidiChannels (midiChannel.getChannels());
                node.setProperty (Tags::midiChannels, midiChannel.getChannels().toMemoryBlock());
            }
        };

        keyLowSlider.addListener (this);
        keyHiSlider.addListener (this);
        transposeSlider.addListener (this);
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
        const auto estimatedH = 4 + (7 * 26) + 2 + 2;
        r = r.withHeight (jmax (estimatedH, r.getHeight()));

        r.removeFromTop (4);
        r.removeFromRight (4);
        layoutComponent (r, nameLabel, nameEditor);
        layoutComponent (r, midiChannelLabel, midiChannel, 
                         midiChannel.getSuggestedHeight (r.getWidth()));
        r.removeFromTop (2);
        layoutComponent (r, midiProgramLabel, midiProgram);
        r.removeFromTop (2);
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

        if (node.isValid() && ! node.isIONode())
        {
            setEnabled (true);
            nameEditor.getTextValue().referTo (node.getPropertyAsValue (Tags::name));
            updateMidiChannels();
            updateSliders();
            updateMidiProgram();

            if (GraphNodePtr ptr = node.getGraphNode())
            {
                ptr->midiProgramChanged.connect (
                    std::bind (&NodeMidiContentView::updateMidiProgram, this));
            }
        }
        else
        {
            if (node.isValid() && node.isIONode())
            {
                nameEditor.getTextValue().referTo (node.getPropertyAsValue (Tags::name));
            }
            else
            {
                nameEditor.getTextValue().referTo (Value());
                nameEditor.setText (String(), false);
            }
            
            setEnabled (false);
        }
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

        if (GraphNodePtr object = node.getGraphNode())
        {
            // use the object because there isn't a notifaction directly back to node model
            // in all cases
            midiProgram.slider.setValue (1 + object->getMidiProgram(), dontSendNotification);
            if (isPositiveAndNotGreaterThan (roundToInt (midiProgram.slider.getValue()), 128))
            {
                midiProgram.loadButton.setEnabled (enabled);
                midiProgram.saveButton.setEnabled (enabled);
                midiProgram.powerButton.setToggleState (enabled, dontSendNotification);
            }
            else
            {
                midiProgram.loadButton.setEnabled (false);
                midiProgram.saveButton.setEnabled (false);
                midiProgram.powerButton.setToggleState (false, dontSendNotification);
            }
        }

        midiProgram.powerButton.setToggleState (node.areMidiProgramsEnabled(), dontSendNotification);
        midiProgram.globalButton.setToggleState (node.useGlobalMidiPrograms(), dontSendNotification);
        midiProgram.globalButton.setEnabled (enabled);
        midiProgram.slider.updateText();
        midiProgram.slider.setEnabled (enabled);
    }
}

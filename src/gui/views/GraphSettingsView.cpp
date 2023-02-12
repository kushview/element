/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <element/services.hpp>
#include "services/engineservice.hpp"
#include "engine/rootgraph.hpp"
#include "engine/velocitycurve.hpp"
#include "gui/properties/MidiMultiChannelPropertyComponent.h"
#include "gui/GuiCommon.h"
#include "gui/views/GraphSettingsView.h"
#include "scopedflag.hpp"

namespace element {
typedef Array<PropertyComponent*> PropertyArray;

class MidiChannelPropertyComponent : public ChoicePropertyComponent
{
public:
    MidiChannelPropertyComponent (const String& name = "MIDI Channel")
        : ChoicePropertyComponent (name)
    {
        choices.add ("Omni");
        choices.add ("");
        for (int i = 1; i <= 16; ++i)
        {
            choices.add (String (i));
        }
    }

    /** midi channel.  0 means omni */
    inline int getMidiChannel() const { return midiChannel; }

    inline int getIndex() const override
    {
        const int index = midiChannel == 0 ? 0 : midiChannel + 1;
        return index;
    }

    inline void setIndex (const int index) override
    {
        midiChannel = (index <= 1) ? 0 : index - 1;
        jassert (isPositiveAndBelow (midiChannel, 17));
        midiChannelChanged();
    }

    virtual void midiChannelChanged() {}

protected:
    int midiChannel = 0;
};

class RenderModePropertyComponent : public ChoicePropertyComponent
{
public:
    RenderModePropertyComponent (const Node& g, const String& name = "Rendering Mode")
        : ChoicePropertyComponent (name), graph (g)
    {
        jassert (graph.isRootGraph());
        choices.add ("Single");
        choices.add ("Parallel");
    }

    inline int getIndex() const override
    {
        const String slug = graph.getProperty (Tags::renderMode, "single").toString();
        return (slug == "single") ? 0 : 1;
    }

    inline void setIndex (const int index) override
    {
        RootGraph::RenderMode mode = index == 0 ? RootGraph::SingleGraph : RootGraph::Parallel;
        graph.setProperty (Tags::renderMode, RootGraph::getSlugForRenderMode (mode));
        if (auto* root = dynamic_cast<RootGraph*> (graph.getObject()))
            root->setRenderMode (mode);

        refresh();
    }

protected:
    Node graph;
};

class VelocityCurvePropertyComponent : public ChoicePropertyComponent
{
public:
    VelocityCurvePropertyComponent (const Node& g)
        : ChoicePropertyComponent ("Velocity Curve"),
          graph (g)
    {
        for (int i = 0; i < VelocityCurve::numModes; ++i)
            choices.add (VelocityCurve::getModeName (i));
    }

    inline int getIndex() const override
    {
        return graph.getProperty ("velocityCurveMode", (int) VelocityCurve::Linear);
    }

    inline void setIndex (const int i) override
    {
        if (! isPositiveAndBelow (i, (int) VelocityCurve::numModes))
            return;

        graph.setProperty ("velocityCurveMode", i);

        if (auto* obj = graph.getObject())
            if (auto* proc = dynamic_cast<RootGraph*> (obj->getAudioProcessor()))
                proc->setVelocityCurveMode ((VelocityCurve::Mode) i);
    }

private:
    Node graph;
    int index;
};

class RootGraphMidiChannels : public MidiMultiChannelPropertyComponent
{
public:
    RootGraphMidiChannels (const Node& g, int proposedWidth)
        : graph (g)
    {
        setSize (proposedWidth, 10);
        setChannels (g.getMidiChannels().get());
        changed.connect (std::bind (&RootGraphMidiChannels::onChannelsChanged, this));
    }

    ~RootGraphMidiChannels()
    {
        changed.disconnect_all_slots();
    }

    void onChannelsChanged()
    {
        if (graph.isRootGraph())
            if (auto* node = graph.getObject())
                if (auto* proc = dynamic_cast<RootGraph*> (node->getAudioProcessor()))
                {
                    proc->setMidiChannels (getChannels());
                    graph.setProperty (Tags::midiChannels, getChannels().toMemoryBlock());
                }
    }

    Node graph;
};

class RootGraphMidiChanel : public MidiChannelPropertyComponent
{
public:
    RootGraphMidiChanel (const Node& n)
        : MidiChannelPropertyComponent(),
          node (n)
    {
        jassert (node.isRootGraph());
        midiChannel = node.getProperty (Tags::midiChannel, 0);
    }

    void midiChannelChanged() override
    {
        auto session = ViewHelpers::getSession (this);
        node.setProperty (Tags::midiChannel, getMidiChannel());
        if (NodeObjectPtr ptr = node.getObject())
            if (auto* root = dynamic_cast<RootGraph*> (ptr->getAudioProcessor()))
                root->setMidiChannel (getMidiChannel());
    }

    Node node;
};

class MidiProgramPropertyComponent : public SliderPropertyComponent,
                                     private Value::Listener
{
public:
    MidiProgramPropertyComponent (const Node& n)
        : SliderPropertyComponent ("MIDI Program", -1.0, 127.0, 1.0, 1.0, false),
          node (n)
    {
        slider.textFromValueFunction = [] (double value) -> String {
            const int iValue = static_cast<int> (value);
            if (iValue < 0)
                return "None";
            return String (1 + iValue);
        };

        slider.valueFromTextFunction = [] (const String& text) -> double {
            if (text == "None")
                return -1.0;
            return static_cast<double> (text.getIntValue()) - 1.0;
        };

        // needed to ensure proper display when first loaded
        slider.updateText();

        programValue = node.getPropertyAsValue (Tags::midiProgram);
        programValue.addListener (this);
    }

    virtual ~MidiProgramPropertyComponent()
    {
        programValue.removeListener (this);
        slider.textFromValueFunction = nullptr;
        slider.valueFromTextFunction = nullptr;
    }

    void setValue (double v) override
    {
        programValue.setValue (roundToInt (v));
        if (auto* root = dynamic_cast<RootGraph*> (node.getObject()))
            root->setMidiProgram ((int) programValue.getValue());
    }

    double getValue() const override
    {
        return (double) node.getProperty (Tags::midiProgram, -1);
    }

private:
    Node node;
    Value programValue;

    void valueChanged (Value& value) override
    {
        if (value.refersToSameSourceAs (programValue))
            slider.setValue ((double) programValue.getValue(), dontSendNotification);
    }
};

class KeyMapPropertyComponent : public PropertyComponent,
                                private Value::Listener
{
public:
    KeyMapPropertyComponent (const Node& n)
        : PropertyComponent ("Key Map", 25),
          textWithButton (*this)
    {
        node = n;
        keyMapValue = node.getPropertyAsValue (Tags::keyMap);
        keyMapValue.addListener (this);
        addAndMakeVisible (textWithButton);
        valueChanged (keyMapValue);
        resized();
    }

    virtual ~KeyMapPropertyComponent()
    {
        listening = false;
        keyMapValue.removeListener (this);
    }

    /** Clears the keymapping and sets the listening state to false */
    void clearMapping()
    {
        keyMapValue.setValue (String());
        listening = false;
        textWithButton.button.setToggleState (false, dontSendNotification);
        textWithButton.text.setText ("", dontSendNotification);
    }

    void refresh() override
    {
        if (keyMapValue.toString() != textWithButton.text.getText())
            valueChanged (keyMapValue);
    }

private:
    Node node;
    Value keyMapValue;
    bool listening = false;

    class TextWithButton : public juce::Component
    {
    public:
        TextWithButton (KeyMapPropertyComponent& o)
            : owner (o)
        {
            addAndMakeVisible (text);
            text.setReadOnly (true);

            addAndMakeVisible (button);
            button.setButtonText ("Map");
            button.onClick = [this]() { owner.buttonClicked(); };
            button.setColour (TextButton::buttonOnColourId, Colors::toggleBlue);

            addAndMakeVisible (clear);
            clear.setButtonText ("Clear");
            clear.onClick = [this]() { owner.clearMapping(); };

            resized();
        }

        ~TextWithButton()
        {
            button.onClick = nullptr;
        }

        void resized() override
        {
            auto b = getLocalBounds();
            clear.setBounds (b.removeFromRight (50));
            b.removeFromRight (4);
            button.setBounds (b.removeFromRight (50));
            b.removeFromRight (4);
            text.setBounds (b);
        }

        KeyMapPropertyComponent& owner;
        TextEditor text;
        TextButton button;
        TextButton clear;
    } textWithButton;

    bool keyPressed (const KeyPress& key) override
    {
        if (! listening)
            return false;

        keyMapValue.setValue (key.getTextDescription());
        return true;
    }

    void buttonClicked()
    {
        listening = ! listening;
        textWithButton.button.setToggleState (listening, dontSendNotification);
        if (listening)
            grabKeyboardFocus();
    }

    void valueChanged (Value& value) override
    {
        if (value.refersToSameSourceAs (keyMapValue))
            textWithButton.text.setText (keyMapValue.toString(), dontSendNotification);
    }
};

class GraphPropertyPanel : public PropertyPanel
{
public:
    GraphPropertyPanel() {}
    ~GraphPropertyPanel()
    {
        clear();
    }

    void setNode (const Node& newNode)
    {
        clear();
        graph = newNode;
        if (graph.isValid() && graph.isGraph())
        {
            PropertyArray props;
            getSessionProperties (props, graph);
            if (useHeader)
                addSection ("Graph Settings", props);
            else
                addProperties (props);
        }
    }

    void setUseHeader (bool header)
    {
        if (useHeader == header)
            return;
        useHeader = header;
        setNode (graph);
    }

private:
    Node graph;
    bool useHeader = true;

    void getSessionProperties (PropertyArray& props, Node g)
    {
        props.add (new TextPropertyComponent (g.getPropertyAsValue (Tags::name),
                                              TRANS ("Name"),
                                              256,
                                              false));
#ifndef EL_SOLO
        props.add (new RenderModePropertyComponent (g));
        props.add (new VelocityCurvePropertyComponent (g));
#endif

        props.add (new RootGraphMidiChannels (g, getWidth() - 100));

#ifndef EL_SOLO
        props.add (new MidiProgramPropertyComponent (g));
#endif
        props.add (new KeyMapPropertyComponent (g));
        // props.add (new BooleanPropertyComponent (g.getPropertyAsValue (Tags::persistent),
        //                                          TRANS("Persistent"),
        //                                          TRANS("Don't unload when deactivated")));
    }
};

GraphSettingsView::GraphSettingsView()
{
    setName ("GraphSettings");
    addAndMakeVisible (props = new GraphPropertyPanel());
    addAndMakeVisible (graphButton);
    graphButton.setTooltip ("Show graph editor");
    graphButton.addListener (this);
    setEscapeTriggersClose (true);

    activeGraphIndex.addListener (this);
}

GraphSettingsView::~GraphSettingsView()
{
    activeGraphIndex.removeListener (this);
}

void GraphSettingsView::setPropertyPanelHeaderVisible (bool useHeader)
{
    props->setUseHeader (useHeader);
}

void GraphSettingsView::setGraphButtonVisible (bool isVisible)
{
    graphButton.setVisible (isVisible);
    resized();
    repaint();
}

void GraphSettingsView::didBecomeActive()
{
    if (isShowing())
        grabKeyboardFocus();
    stabilizeContent();
}

void GraphSettingsView::stabilizeContent()
{
    if (auto* const world = ViewHelpers::getGlobals (this))
    {
        props->setNode (world->getSession()->getCurrentGraph());
    }

    if (auto session = ViewHelpers::getSession (this))
    {
        if (! activeGraphIndex.refersToSameSourceAs (session->getActiveGraphIndexObject()))
        {
            ScopedFlag flag (updateWhenActiveGraphChanges, false);
            activeGraphIndex.referTo (session->getActiveGraphIndexObject());
        }
    }
}

void GraphSettingsView::resized()
{
    props->setBounds (getLocalBounds().reduced (2));
    const int configButtonSize = 14;
    graphButton.setBounds (getWidth() - configButtonSize - 4, 4, configButtonSize, configButtonSize);
}

void GraphSettingsView::buttonClicked (Button* button)
{
    if (button == &graphButton)
        if (auto* const world = ViewHelpers::getGlobals (this))
            world->getCommandManager().invokeDirectly (Commands::showGraphEditor, true);
}

void GraphSettingsView::setUpdateOnActiveGraphChange (bool shouldUpdate)
{
    if (updateWhenActiveGraphChanges == shouldUpdate)
        return;
    updateWhenActiveGraphChanges = shouldUpdate;
}

void GraphSettingsView::valueChanged (Value& value)
{
    if (updateWhenActiveGraphChanges && value.refersToSameSourceAs (value))
        stabilizeContent();
}
} // namespace element

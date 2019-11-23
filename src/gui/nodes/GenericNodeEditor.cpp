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

#include "gui/nodes/GenericNodeEditor.h"
#include "gui/LookAndFeel.h"

namespace Element {

namespace GenericEditorHelpers {

static AudioProcessor* getAudioProcessor (const Node& node)
{
    if (auto* obj = node.getGraphNode())
        return obj->getAudioProcessor();
    return nullptr;
}

}

class BooleanParameterComponent final   : public Component,
                                          private ParameterListener
{
public:
    BooleanParameterComponent (Parameter* param)
        : ParameterListener (param)
    {
        // Set the initial value.
        handleNewParameterValue();
        button.onClick = [this]() { buttonClicked(); };
        addAndMakeVisible (button);
    }

    void paint (Graphics&) override {}

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromLeft (8);
        button.setBounds (area.reduced (0, 10));
    }

private:
    void handleNewParameterValue() override
    {
        auto parameterState = getParameterState (getParameter()->getValue());

        if (button.getToggleState() != parameterState)
            button.setToggleState (parameterState, dontSendNotification);
    }

    void buttonClicked()
    {
        if (getParameterState (getParameter()->getValue()) != button.getToggleState())
        {
            getParameter()->beginChangeGesture();
            getParameter()->setValueNotifyingHost (button.getToggleState() ? 1.0f : 0.0f);
            getParameter()->endChangeGesture();
        }
    }

    bool getParameterState (float value) const noexcept
    {
        return value >= 0.5f;
    }

    ToggleButton button;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BooleanParameterComponent)
};

class SwitchParameterComponent final   : public Component,
                                         private ParameterListener
{
public:
    SwitchParameterComponent (Parameter* param)
        : ParameterListener (param)
    {
        auto* leftButton  = buttons.add (new TextButton());
        auto* rightButton = buttons.add (new TextButton());

        for (auto* button : buttons)
        {
            button->setRadioGroupId (293847);
            button->setClickingTogglesState (true);
        }

        leftButton ->setButtonText (getParameter()->getText (0.0f, 16));
        rightButton->setButtonText (getParameter()->getText (1.0f, 16));

        leftButton ->setConnectedEdges (Button::ConnectedOnRight);
        rightButton->setConnectedEdges (Button::ConnectedOnLeft);

        // Set the initial value.
        leftButton->setToggleState (true, dontSendNotification);
        handleNewParameterValue();

        rightButton->onStateChange = [this]() { rightButtonChanged(); };

        for (auto* button : buttons)
            addAndMakeVisible (button);
    }

    void paint (Graphics&) override {}

    void resized() override
    {
        auto area = getLocalBounds().reduced (0, 8);
        area.removeFromLeft (8);

        for (auto* button : buttons)
            button->setBounds (area.removeFromLeft (80));
    }

private:
    void handleNewParameterValue() override
    {
        bool newState = getParameterState();

        if (buttons[1]->getToggleState() != newState)
        {
            buttons[1]->setToggleState (newState,   dontSendNotification);
            buttons[0]->setToggleState (! newState, dontSendNotification);
        }
    }

    void rightButtonChanged()
    {
        auto buttonState = buttons[1]->getToggleState();

        if (getParameterState() != buttonState)
        {
            getParameter()->beginChangeGesture();

            if (getParameter()->getValueStrings().isEmpty())
            {
                getParameter()->setValueNotifyingHost (buttonState ? 1.0f : 0.0f);
            }
            else
            {
                // When a parameter provides a list of strings we must set its
                // value using those strings, rather than a float, because VSTs can
                // have uneven spacing between the different allowed values and we
                // want the snapping behaviour to be consistent with what we do with
                // a combo box.
                String selectedText = buttonState ? buttons[1]->getButtonText() : buttons[0]->getButtonText();
                getParameter()->setValueNotifyingHost (getParameter()->getValueForText (selectedText));
            }

            getParameter()->endChangeGesture();
        }
    }

    bool getParameterState()
    {
        if (getParameter()->getValueStrings().isEmpty())
            return getParameter()->getValue() > 0.5f;

        auto index = getParameter()->getValueStrings()
            .indexOf (getParameter()->getCurrentValueAsText());

        if (index < 0)
        {
            // The parameter is producing some unexpected text, so we'll do
            // some linear interpolation.
            index = roundToInt (getParameter()->getValue());
        }

        return index == 1;
    }

    OwnedArray<TextButton> buttons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwitchParameterComponent)
};

class ChoiceParameterComponent final   : public Component,
                                         private ParameterListener
{
public:
    ChoiceParameterComponent (Parameter* param)
        : ParameterListener (param),
          parameterValues (getParameter()->getValueStrings())
    {
        box.addItemList (parameterValues, 1);

        // Set the initial value.
        handleNewParameterValue();

        box.onChange = [this]() { boxChanged(); };
        addAndMakeVisible (box);
    }

    void paint (Graphics&) override {}

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromLeft (8);
        box.setBounds (area.reduced (0, 10));
    }

private:
    void handleNewParameterValue() override
    {
        auto index = parameterValues.indexOf (getParameter()->getCurrentValueAsText());

        if (index < 0)
        {
            // The parameter is producing some unexpected text, so we'll do
            // some linear interpolation.
            index = roundToInt (getParameter()->getValue() * (parameterValues.size() - 1));
        }

        box.setSelectedItemIndex (index);
    }

    void boxChanged()
    {
        if (getParameter()->getCurrentValueAsText() != box.getText())
        {
            getParameter()->beginChangeGesture();

            // When a parameter provides a list of strings we must set its
            // value using those strings, rather than a float, because VSTs can
            // have uneven spacing between the different allowed values.
            getParameter()->setValueNotifyingHost (getParameter()->getValueForText (box.getText()));

            getParameter()->endChangeGesture();
        }
    }

    ComboBox box;
    const StringArray parameterValues;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceParameterComponent)
};

class SliderParameterComponent final   : public Component,
                                         private ParameterListener
{
public:
    SliderParameterComponent (Parameter* param)
        : ParameterListener (param)
    {
        if (getParameter()->getNumSteps() != AudioProcessor::getDefaultNumParameterSteps())
            slider.setRange (0.0, 1.0, 1.0 / (getParameter()->getNumSteps() - 1.0));
        else
            slider.setRange (0.0, 1.0);

        slider.setScrollWheelEnabled (false);
        addAndMakeVisible (slider);

        valueLabel.setFont (Font (12.f));
        valueLabel.setColour (Label::outlineColourId, slider.findColour (Slider::textBoxOutlineColourId));
        valueLabel.setBorderSize ({ 1, 1, 1, 1 });
        valueLabel.setJustificationType (Justification::centred);
        addAndMakeVisible (valueLabel);

        // Set the initial value.
        handleNewParameterValue();

        slider.onValueChange = [this]() { sliderValueChanged(); };
        slider.onDragStart   = [this]() { sliderStartedDragging(); };
        slider.onDragEnd     = [this]() { sliderStoppedDragging(); };
    }

    void paint (Graphics&) override {}

    void resized() override
    {
        auto area = getLocalBounds().reduced (0, 10);

        valueLabel.setBounds (area.removeFromRight (80));

        area.removeFromLeft (6);
        slider.setBounds (area);
    }

private:
    void updateTextDisplay()
    {
        jassert(MessageManager::getInstance()->isThisTheMessageThread());
        valueLabel.setText (getParameter()->getCurrentValueAsText(), dontSendNotification);
    }

    void handleNewParameterValue() override
    {
        jassert(MessageManager::getInstance()->isThisTheMessageThread());
        if (! isDragging)
        {
            slider.setValue (getParameter()->getValue(), dontSendNotification);
            updateTextDisplay();
        }
    }

    void sliderValueChanged()
    {
        auto newVal = (float) slider.getValue();

        if (getParameter()->getValue() != newVal)
        {
            if (! isDragging)
                getParameter()->beginChangeGesture();

            getParameter()->setValueNotifyingHost ((float) slider.getValue());
            updateTextDisplay();

            if (! isDragging)
                getParameter()->endChangeGesture();
        }
    }

    void sliderStartedDragging()
    {
        isDragging = true;
        getParameter()->beginChangeGesture();
    }

    void sliderStoppedDragging()
    {
        isDragging = false;
        getParameter()->endChangeGesture();
    }

    Slider slider { Slider::LinearHorizontal, Slider::TextEntryBoxPosition::NoTextBox };
    Label valueLabel;
    bool isDragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderParameterComponent)
};

class ParameterDisplayComponent   : public Component
{
public:
    ParameterDisplayComponent (Parameter* param)
    {
        parameterName.setFont (Font (12.f));
        parameterName.setText (param->getName (128), dontSendNotification);
        parameterName.setJustificationType (Justification::centredRight);
        addAndMakeVisible (parameterName);

        parameterLabel.setText (param->getLabel(), dontSendNotification);
        addAndMakeVisible (parameterLabel);

        if (param->isBoolean())
        {
            // The AU, AUv3 and VST (only via a .vstxml file) SDKs support
            // marking a parameter as boolean. If you want consistency across
            // all  formats then it might be best to use a
            // SwitchParameterComponent instead.
            parameterComp.reset (new BooleanParameterComponent (param));
        }
        else if (param->getNumSteps() == 2)
        {
            // Most hosts display any parameter with just two steps as a switch.
            parameterComp.reset (new SwitchParameterComponent (param));
        }
        else if (! param->getValueStrings().isEmpty())
        {
            // If we have a list of strings to represent the different states a
            // parameter can be in then we should present a dropdown allowing a
            // user to pick one of them.
            parameterComp.reset (new ChoiceParameterComponent (param));
        }
        else
        {
            // Everything else can be represented as a slider.
            parameterComp.reset (new SliderParameterComponent (param));
        }

        addAndMakeVisible (parameterComp.get());

        setSize (400, 34);
    }

    void paint (Graphics&) override {}

    void resized() override
    {
        auto area = getLocalBounds();

        parameterName.setBounds (area.removeFromLeft (80));
        parameterLabel.setBounds (area.removeFromRight (30));
        parameterComp->setBounds (area);
    }

private:
    Label parameterName, parameterLabel;
    std::unique_ptr<Component> parameterComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterDisplayComponent)
};

class ParametersPanel   : public Component
{
public:
    ParametersPanel (const ParameterArray& params)
    {
        for (auto* param : params)
            if (param->isAutomatable())
                addAndMakeVisible (paramComponents.add (new ParameterDisplayComponent (param)));

        if (auto* comp = paramComponents [0])
            setSize (comp->getWidth(), comp->getHeight() * paramComponents.size());
        else
            setSize (400, 100);
    }

    void paint (Graphics& g) override { }

    void resized() override
    {
        auto area = getLocalBounds();

        for (auto* comp : paramComponents)
            comp->setBounds (area.removeFromTop (comp->getHeight()));
    }

private:
    OwnedArray<ParameterDisplayComponent> paramComponents;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametersPanel)
};

struct GenericNodeEditor::Pimpl
{
    Pimpl (GenericNodeEditor& parent)
        : owner (parent)
    {
        GraphNodePtr ptr = parent.getNodeObject();
        jassert (ptr != nullptr);
        owner.setOpaque (true);
        view.setViewedComponent (new ParametersPanel (ptr->getParameters()));
        owner.addAndMakeVisible (view);
        view.setScrollBarsShown (true, false);
    }

    GenericNodeEditor& owner;
    Viewport view;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

GenericNodeEditor::GenericNodeEditor (const Node& node)
    : NodeEditorComponent (node), pimpl (new Pimpl (*this))
{
    jassert (nullptr != GenericEditorHelpers::getAudioProcessor (node));  // only nodes with real parameters are supported
    setSize (pimpl->view.getViewedComponent()->getWidth() + pimpl->view.getVerticalScrollBar().getWidth(),
             jmin (pimpl->view.getViewedComponent()->getHeight(), 400));
}

GenericNodeEditor::~GenericNodeEditor() { }

void GenericNodeEditor::paint (Graphics& g)
{ 
    g.fillAll (Element::LookAndFeel::backgroundColor);
}

void GenericNodeEditor::resized()
{
    if (auto* const comp = pimpl->view.getViewedComponent())
        comp->setSize (jmax (150, getWidth()), comp->getHeight());
    pimpl->view.setBounds (getLocalBounds());
}

}

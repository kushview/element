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

#pragma once

#include "ElementApp.h"
#include "PluginProcessor.h"
#include "gui/ContentComponent.h"
#include "gui/LookAndFeel.h"
#include "Signals.h"

namespace Element {

// class ContentComponent;
class ActivationComponent;

class PerformanceParameterSlider : public Slider,
                                   private AudioProcessorParameter::Listener,
                                   private Timer
{
public:
    PerformanceParameterSlider (PerformanceParameter& p)
        : param (p)
    {
        param.addListener (this);
        setSliderStyle (Slider::RotaryHorizontalDrag);
        setTextBoxStyle (Slider::NoTextBox, false, 10, 10);
        setScrollWheelEnabled (false);

        if (param.getNumSteps() != AudioProcessor::getDefaultNumParameterSteps())
            setRange (0.0, 1.0, 1.0 / (param.getNumSteps() - 1.0));
        else
            setRange (0.0, 1.0);

        handleNewParameterValue();
        
        onValueChange = [this]() { sliderValueChanged(); };
        onDragStart   = [this]() { sliderStartedDragging(); };
        onDragEnd     = [this]() { sliderStoppedDragging(); };
        
        updateEnablement();
        updateToolTip();

        param.onCleared = [this]()
        {
            updateEnablement();
            updateToolTip();
        };

        startTimerHz (100);
    }
    
    ~PerformanceParameterSlider()
    {
        param.onCleared = nullptr;
        param.removeListener (this);
    }
    
    void updateToolTip()
    {
        if (! param.haveNode())
        {
            setTooltip (param.getName (100));
            return;
        }
        
        String message = param.getNode().getName();
        String paramName = param.getBoundParameterName();
        if (paramName.isEmpty())
            paramName = param.getName (100);
        
        if (message.isNotEmpty() && paramName.isNotEmpty())
            message << " - " << paramName;
        else if (paramName.isNotEmpty())
            message = paramName;
        
        setTooltip (message);
    }

    void updateEnablement()
    {
        setEnabled (param.haveNode());
    }

    void mouseDown (const MouseEvent& ev) override;
    
private:
    PerformanceParameter& param;
    Atomic<int> parameterValueHasChanged { 0 };
    bool isDragging = false;

    //==============================================================================
    void parameterValueChanged (int, float) override
    {
        parameterValueHasChanged = 1;
    }

    void parameterGestureChanged (int, bool) override {}

    //==============================================================================
    void timerCallback() override
    {
        if (parameterValueHasChanged.compareAndSetBool (0, 1))
        {
            handleNewParameterValue();
            startTimerHz (50);
        }
        else
        {
            startTimer (jmin (250, getTimerInterval() + 10));
        }
    }

    void updateTextDisplay()
    {
        jassert(MessageManager::getInstance()->isThisTheMessageThread());
        // valueLabel.setText (getParameter().getCurrentValueAsText(), dontSendNotification);
    }

    void handleNewParameterValue()
    {
        jassert(MessageManager::getInstance()->isThisTheMessageThread());
        if (! isDragging)
        {
            setValue (param.getValue(), dontSendNotification);
            updateTextDisplay();
        }
    }

    void sliderValueChanged()
    {
        auto newVal = (float) getValue();

        if (param.getValue() != newVal)
        {
            if (! isDragging)
                param.beginChangeGesture();

            param.setValueNotifyingHost (newVal);
            updateTextDisplay();

            if (! isDragging)
                param.endChangeGesture();
        }
    }

    void sliderStartedDragging()
    {
        isDragging = true;
        param.beginChangeGesture();
    }

    void sliderStoppedDragging()
    {
        isDragging = false;
        param.endChangeGesture();
    }
};

class PerfSliders : public Component
{
public:
    PerfSliders (ElementPluginAudioProcessor& processor)
    {
        for (auto* param : processor.getParameters())
        {
            if (auto* perf = dynamic_cast<PerformanceParameter*> (param))
            {
                auto* slider = sliders.add (new PerformanceParameterSlider (*perf));
                addAndMakeVisible (slider);
            }
        }

        setSize (500, 44);
    }

    void update()
    {
        for (auto* const slider : sliders)
        {
            slider->updateEnablement();
            slider->updateToolTip();
        }
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Element::LookAndFeel::widgetBackgroundColor.darker (0.29));
    }

    void resized() override
    {
        int space = 12;
        Rectangle<int> sb;
        sb = getLocalBounds();
        
       #if 0
        // right to left
        sb.removeFromRight (space);
        for (int i = sliders.size(); --i >= 0;)
        {
            auto* const slider = sliders [i];
            slider->setBounds (sb.removeFromRight (44).reduced(4));
            sb.removeFromRight (space);
        }
       #else
        // left to right
        sb.removeFromLeft (space);
        for (int i = 0; i < sliders.size(); ++i)
        {
            auto* const slider = sliders [i];
            slider->setBounds (sb.removeFromLeft (44).reduced (4));
            sb.removeFromLeft (space);
        }
       #endif
    }

private:
    OwnedArray<PerformanceParameterSlider> sliders;
};

class ElementPluginAudioProcessorEditor  : public AudioProcessorEditor,
                                           public KeyListener,
                                           public AsyncUpdater
{
public:
    ElementPluginAudioProcessorEditor (ElementPluginAudioProcessor&);
    ~ElementPluginAudioProcessorEditor();

    void paint (Graphics&) override;
    void resized() override;

    bool keyPressed (const KeyPress &key, Component *originatingComponent) override;
    bool keyStateChanged (bool isKeyDown, Component *originatingComponent) override { return true; }
    
    Element::ContentComponent* getContentComponent();
    
    void handleAsyncUpdate() override;
    ElementPluginAudioProcessor& getProcessor() { return processor; }

    void updatePerformanceParamEnablements()
    {
        if (auto* cc = dynamic_cast<ContentComponent*> (content.getComponent()))
            if (auto* ps = dynamic_cast<PerfSliders*> (cc->getExtraView()))
                ps->update();
    }

    void setWantsPluginKeyboardFocus (bool focus)
    {
        if (focus == getWantsKeyboardFocus())
            return;

        setWantsKeyboardFocus (focus);
        processor.setEditorWantsKeyboard (focus);
        
        if (getWantsKeyboardFocus())
        {
            addKeyListener (this);
        }
        else
        {
            removeKeyListener (this);
        }
    }

    bool getWantsPluginKeyboardFocus() const
    {
        return getWantsKeyboardFocus();
    }

private:
    ElementPluginAudioProcessor& processor;
    SafePointer<Component> content;
    SignalConnection perfParamChangedConnection;

    const int paramTableSize = 100;
    bool paramTableVisible = false;
    class ParamTable; std::unique_ptr<ParamTable> paramTable;
    class ParamTableToggle; std::unique_ptr<ParamTableToggle> paramToggle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ElementPluginAudioProcessorEditor)
};
}

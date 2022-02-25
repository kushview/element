/*
    Author: Jatin Chowdhury (jatin@ccrma.stanford.edu)
    
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

#include "KnobsComponent.h"
#include "../LookAndFeel.h"

namespace Element {

KnobsComponent::KnobsComponent (AudioProcessor& proc, std::function<void()> paramLambda)
{
    auto setupSlider = [=] (AudioParameterFloat* param, String suffix = {}) {
        Slider* newSlider = new Slider;

        // set slider style
        addAndMakeVisible (newSlider);
        newSlider->setTextValueSuffix (suffix);
        newSlider->setSliderStyle (Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        newSlider->setName (param->name);
        newSlider->setNumDecimalPlacesToDisplay (2);
        newSlider->setTextBoxStyle (Slider::TextBoxBelow, false, 75, 18);
        newSlider->setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);

        // connect slider to parameter
        newSlider->setRange (Range<double> ((double) param->range.getRange().getStart(), (double) param->range.getRange().getEnd()), 0.01);
        newSlider->setSkewFactor ((double) param->range.skew);
        newSlider->setValue ((double) *param, dontSendNotification);
        newSlider->setDoubleClickReturnValue (true, param->convertFrom0to1 (dynamic_cast<AudioProcessorParameterWithID*> (param)->getDefaultValue()));
        newSlider->onDragStart = [param] { param->beginChangeGesture(); };
        newSlider->onDragEnd = [param] { param->endChangeGesture(); };
        newSlider->onValueChange = [=] {
            param->setValueNotifyingHost (param->convertTo0to1 ((float) newSlider->getValue()));
            paramLambda();
        };

        sliders.add (newSlider);
    };

    auto setupBox = [=] (AudioParameterChoice* param) {
        ComboBox* newBox = new ComboBox;

        addAndMakeVisible (newBox);
        newBox->setName (param->name);
        newBox->addItemList (param->choices, 1);
        newBox->setSelectedItemIndex ((int) *param, dontSendNotification);
        newBox->onChange = [=] {
            *param = newBox->getSelectedItemIndex();
            paramLambda();
        };

        boxes.add (newBox);
    };

    const auto params = proc.getParameters();

    for (auto* param : params)
    {
        if (auto* paramFloat = dynamic_cast<AudioParameterFloat*> (param))
        {
            // set up units
            String suffix;
            if (paramFloat->name.contains ("[Hz]"))
                suffix = " Hz";
            else if (paramFloat->name.contains ("[dB]"))
                suffix = " dB";
            else if (paramFloat->name.contains ("[ms]"))
                suffix = " Ms";

            setupSlider (paramFloat, suffix);
        }

        else if (auto* paramChoice = dynamic_cast<AudioParameterChoice*> (param))
        {
            setupBox (paramChoice);
        }
    }
}

void KnobsComponent::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (Style::widgetBackgroundColorId));

    // Print names for sliders and combo boxes
    g.setColour (Colours::white);
    const int height = 20;
    auto makeName = [this, &g, height] (const Component& comp, String name) {
        Rectangle<int> nameBox (comp.getX(), 2, comp.getWidth(), height);
        g.drawFittedText (name, nameBox, Justification::centred, 1);
    };

    for (auto* s : sliders)
        makeName (*s, s->getName().upToFirstOccurrenceOf (" [", false, false));

    for (auto* b : boxes)
        makeName (*b, b->getName());
}

void KnobsComponent::resized()
{
    int x = 5;
    bool first = true;
    for (auto* s : sliders)
    {
        int offset = first ? -3 : -15;
        s->setBounds (x + offset, 20, 100, 75);
        x = s->getRight();
        first = false;
    }

    for (auto* b : boxes)
    {
        int offset = first ? 5 : 0;
        b->setBounds (x + offset, 40, 90, 25);
        x = b->getRight();
        first = false;
    }
}

} // namespace Element

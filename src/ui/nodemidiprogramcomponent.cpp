// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "ui/nodemidiprogramcomponent.hpp"

namespace element {

NodeMidiProgramComponent::NodeMidiProgramComponent()
{
    addAndMakeVisible (name);
    name.setText ("Program name...", dontSendNotification);
    name.setTooltip ("MIDI Program name");
    name.setFont (Font (12.f));
    name.setEditable (false, true, false);

    addAndMakeVisible (slider);
    slider.setSliderStyle (Slider::IncDecButtons);
    slider.setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    slider.setRange (1.0, 128.0, 1.0);

    addAndMakeVisible (loadButton);
    loadButton.setIcon (Icon (getIcons().farRedoAlt, Colors::textColor), 11.6f);
    addAndMakeVisible (saveButton);
    saveButton.setIcon (Icon (getIcons().farSave, Colors::textColor));
    addAndMakeVisible (trashButton);
    trashButton.setIcon (Icon (getIcons().farTrashAlt, Colors::textColor));

    addAndMakeVisible (globalButton);
    globalButton.setTooltip ("Use global MIDI programs");
    globalButton.setColour (TextButton::buttonOnColourId, Colors::toggleGreen);
    globalButton.setClickingTogglesState (true);
    globalButton.setIcon (Icon (getIcons().farGlobe, Colors::textColor));

    addAndMakeVisible (powerButton);
    powerButton.setTooltip ("Enable/disable MIDI programs");
    powerButton.setColour (TextButton::buttonOnColourId, Colors::toggleBlue);
    powerButton.setClickingTogglesState (true);
    powerButton.setIcon (Icon (getIcons().fasPowerOff, Colors::textColor));
}

void NodeMidiProgramComponent::resized()
{
    auto r = getLocalBounds();
    auto r2 = r.removeFromTop (r.getHeight() / 2);
    r = r.withWidth (jmax (100 + 48, r.getWidth()));
    powerButton.setBounds (r.removeFromRight (20));
    r.removeFromRight (1);
    globalButton.setBounds (r.removeFromRight (20));
    r.removeFromRight (1);
    trashButton.setBounds (r.removeFromRight (20));
    r.removeFromRight (1);
    loadButton.setBounds (r.removeFromRight (20));
    r.removeFromRight (1);
    saveButton.setBounds (r.removeFromRight (20));
    slider.setBounds (r.removeFromLeft (108));

    name.setBounds (r2);
}

} // namespace element
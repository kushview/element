// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/nodemidiprogramcomponent.hpp"

namespace element {

NodeMidiProgramComponent::NodeMidiProgramComponent()
{
    addAndMakeVisible (addButton);
    addButton.setButtonText ("+");

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
    powerButton.setBounds (r.removeFromRight (20));
    r.removeFromRight (1);
    globalButton.setBounds (r.removeFromRight (20));
    r.removeFromRight (1);
    addButton.setBounds (r.removeFromRight (24));
}

} // namespace element
// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/buttons.hpp"

namespace element {

struct NodeMidiProgramComponent : public Component
{
    juce::TextButton addButton;
    IconButton globalButton;
    IconButton powerButton;

    NodeMidiProgramComponent();
    void resized() override;
};

} // namespace element

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "ui/buttons.hpp"

namespace element {

struct NodeMidiProgramComponent : public Component
{
    Label name;
    Slider slider;
    IconButton loadButton;
    IconButton saveButton;
    IconButton globalButton;
    IconButton powerButton;
    IconButton trashButton;

    NodeMidiProgramComponent();
    void resized() override;
};

} // namespace element

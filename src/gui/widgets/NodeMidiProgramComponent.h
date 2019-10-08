#pragma once

#include "gui/Buttons.h"

namespace Element {

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

}

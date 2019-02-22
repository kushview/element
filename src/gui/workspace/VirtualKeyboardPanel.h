#pragma once

#include "gui/VirtualKeyboardView.h"
#include "gui/workspace/ContentViewPanel.h"

namespace Element {

class VirtualKeyboardPanel : public ContentViewPanel<VirtualKeyboardView>
{
public:
    VirtualKeyboardPanel() { setName ("Virtual Keyboard"); }
    ~VirtualKeyboardPanel() = default;
};

}

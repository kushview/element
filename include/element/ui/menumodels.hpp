// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/gui_basics.hpp>

namespace element {

class MainMenuBarModel : public juce::MenuBarModel {
public:
    virtual ~MainMenuBarModel() = default;
    virtual juce::PopupMenu* getMacAppMenu() { return nullptr; }
};
} // namespace element

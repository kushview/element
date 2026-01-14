// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/element.hpp>
#include <element/juce/gui_basics.hpp>

namespace element {

class EL_API View : public juce::Component {
public:
    View() = default;
    virtual ~View() = default;
};

} // namespace element

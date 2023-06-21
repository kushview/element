// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/core.hpp>

namespace element {

class NodeProxy : public juce::ReferenceCountedObject {
public:
    NodeProxy() = delete;
    virtual ~NodeProxy() = default;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeProxy)
};

} // namespace element
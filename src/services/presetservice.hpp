// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/services.hpp>

namespace element {

class Node;

class PresetService : public Service
{
public:
    PresetService();
    ~PresetService();
    void activate() override;
    void deactivate() override;

    void refresh();
    void add (const Node& Node, const juce::String& presetName = juce::String());

private:
    friend struct Impl;
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace element

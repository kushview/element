// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/nodeeditor.hpp>

namespace element {

class Node;
class GuiService;

class VolumeNodeEditor : public NodeEditor
{
public:
    VolumeNodeEditor (const Node&, GuiService&);
    ~VolumeNodeEditor();

    void paint (Graphics& g) override;
    void resized() override;

private:
    class ChannelStrip;
    friend class ChannelStrip;
    std::unique_ptr<ChannelStrip> strip;
};

} // namespace element

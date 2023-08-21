// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later
// Author: Eliot Akira <me@eliotakira.com>

#pragma once

#include <element/ui/nodeeditor.hpp>

namespace element {

class MidiMonitorNodeEditor : public NodeEditor
{
public:
    MidiMonitorNodeEditor (const Node& node);
    virtual ~MidiMonitorNodeEditor();
    void paint (Graphics& g) override { g.fillAll (findColour (TextEditor::backgroundColourId).darker()); }
    void resized() override;

private:
    class Logger;
    std::unique_ptr<Logger> logger;
    TextButton clearButton;
};

} // namespace element

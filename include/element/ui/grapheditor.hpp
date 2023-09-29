// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/nodeeditor.hpp>

namespace element {

class GraphNodeEditor : public NodeEditor {
public:
    GraphNodeEditor (const Node& graph);
    ~GraphNodeEditor();
    void resized() override;
    void paint (juce::Graphics& g) override;
    
private:
    class Graph;
    std::unique_ptr<Graph> graph;
};

} // namespace element

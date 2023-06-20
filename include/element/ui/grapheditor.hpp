// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/nodeeditor.hpp>

namespace element {

class GraphEditor : public NodeEditorComponent {
public:
    GraphEditor (const Node& graph);
    ~GraphEditor();
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    class Graph;
    std::unique_ptr<Graph> graph;
};

} // namespace element

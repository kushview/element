// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/grapheditor.hpp>
#include "gui/GraphEditorComponent.h"

namespace element {

class GraphEditor::Graph : public GraphEditorComponent
{
public:
    Graph (GraphEditor& ge) : editor (ge) {}
    ~Graph() {}

    GraphEditor& editor;
};

GraphEditor::GraphEditor (const Node& n)
    : NodeEditor (n)
{
    setOpaque (true);
    graph = std::make_unique<Graph> (*this);
    addAndMakeVisible (graph.get());
    setSize (500, 290);
    graph->setNode (getNode());
}

GraphEditor::~GraphEditor()
{
    graph.reset();
}

void GraphEditor::resized()
{
    graph->setBounds (getLocalBounds().reduced (1));
}

void GraphEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

} // namespace element

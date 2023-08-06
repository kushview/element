// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/grapheditor.hpp>
#include "gui/GraphEditorComponent.h"

namespace element {

class GraphNodeEditor::Graph : public GraphEditorComponent
{
public:
    Graph (GraphNodeEditor& ge) : editor (ge) {}
    ~Graph() {}

    GraphNodeEditor& editor;
};

GraphNodeEditor::GraphNodeEditor (const Node& n)
    : NodeEditor (n)
{
    setOpaque (true);
    graph = std::make_unique<Graph> (*this);
    addAndMakeVisible (graph.get());
    setSize (500, 290);
    graph->setNode (getNode());
}

GraphNodeEditor::~GraphNodeEditor()
{
    graph.reset();
}

void GraphNodeEditor::resized()
{
    graph->setBounds (getLocalBounds().reduced (1));
}

void GraphNodeEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

} // namespace element

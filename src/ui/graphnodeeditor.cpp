// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/grapheditor.hpp>
#include "ui/grapheditorcomponent.hpp"

namespace element {

namespace detail {
inline static ValueTree stateData (const Node& node)
{
    return node.getUIValueTree().getOrCreateChildWithName ("GraphNodeEditor", nullptr);
}
} // namespace detail

class GraphNodeEditor::Graph : public GraphEditor
{
public:
    Graph (GraphNodeEditor& ge) : editor (ge) {}
    ~Graph() {}

    GraphNodeEditor& editor;
    Rectangle<int> lastSize;
};

GraphNodeEditor::GraphNodeEditor (const Node& n)
    : NodeEditor (n)
{
    setOpaque (true);
    setResizable (true);
    graph = std::make_unique<Graph> (*this);
    addAndMakeVisible (graph.get());

    auto data = detail::stateData (getNode());
    auto r = Rectangle<int>::fromString (data.getProperty ("size", "").toString());
    if (r.isEmpty())
        setSize (710, 440);
    else
        setSize (r.getWidth(), r.getHeight());
    graph->setNode (getNode());
}

GraphNodeEditor::~GraphNodeEditor()
{
    auto data = detail::stateData (getNode());
    data.setProperty ("size", graph->lastSize.toString(), nullptr);

    graph.reset();
}

void GraphNodeEditor::resized()
{
    graph->lastSize = getLocalBounds();
    graph->setBounds (getLocalBounds().reduced (1));
}

void GraphNodeEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

} // namespace element

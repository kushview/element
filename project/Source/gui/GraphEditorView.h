/*
    GraphEditorView.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "gui/GraphDisplayView.h"
#include "gui/GraphEditorComponent.h"

namespace Element {

class GraphEditorView : public GraphDisplayView
{
public:
    GraphEditorView();
    ~GraphEditorView();

    void didBecomeActive() override;
    void stabilizeContent() override;
    void paint (Graphics& g) override;

protected:
    void graphDisplayResized (const Rectangle<int>& area) override;
    void graphNodeChanged (const Node& g, const Node&) override;

private:
    Node node;
    GraphEditorComponent graph;
};

}

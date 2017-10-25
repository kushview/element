/*
    GraphEditorView.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "gui/GuiCommon.h"
#include "gui/GraphEditorView.h"

namespace Element {
    
    GraphEditorView::GraphEditorView()
    {
        setName ("GraphEditor");
        addAndMakeVisible (graph);
    }
    
    GraphEditorView::~GraphEditorView() { }

    void GraphEditorView::stabilizeContent()
    {
        graph.deleteAllChildren();
        if (auto session = ViewHelpers::getSession (this))
            graph.setNode (session->getCurrentGraph());
        graph.updateComponents();
    }
    
    void GraphEditorView::didBecomeActive()
    {
        stabilizeContent();
    }
    
    void GraphEditorView::paint (Graphics& g)
    {
        g.fillAll (LookAndFeel::contentBackgroundColor);
    }
    
    void GraphEditorView::resized()
    {
        graph.setBounds (getLocalBounds());
    }
}

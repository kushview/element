/*
    GraphEditorView.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "gui/GuiCommon.h"
#include "gui/GraphEditorView.h"

namespace Element {
    
    GraphEditorView::GraphEditorView()
    {
        addAndMakeVisible (graph);
    }
    
    GraphEditorView::~GraphEditorView()
    {
        DBG("graph ed view");
    }

    void GraphEditorView::stabilizeContent()
    {
        graph.removeAllChildren();
        if (auto session = ViewHelpers::getSession (this)) {
         
            graph.setNode (session->getCurrentGraph());
        }
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

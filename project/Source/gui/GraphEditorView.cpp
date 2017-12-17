/*
    GraphEditorView.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "gui/GuiCommon.h"
#include "gui/GraphEditorView.h"
#include "Globals.h"
#include "session/UnlockStatus.h"

namespace Element {
    
    GraphEditorView::GraphEditorView()
    {
        setName ("GraphEditor");
        addAndMakeVisible (graph);
    }
    
    GraphEditorView::~GraphEditorView() { }

    void GraphEditorView::setNode (const Node& g)
    {
        if (g == node)
            return;

        node = g;
        stabilizeContent();
    }

    void GraphEditorView::stabilizeContent()
    {
        if (node.isGraph() && !node.isRootGraph())
        {
            disableIfNotUnlocked();
        }
        else 
        {
            setEnabled (true); 
            setInterceptsMouseClicks (true, true);
        }

        graph.setNode (node);
    }
    
    void GraphEditorView::didBecomeActive()
    {
        if (!node.isValid() || !node.isGraph())
            if (auto session = ViewHelpers::getSession (this))
                node = session->getCurrentGraph();
        stabilizeContent();
    }
    
    void GraphEditorView::paint (Graphics& g)
    {
        g.fillAll (LookAndFeel::contentBackgroundColor);
    }
    
    void GraphEditorView::resized()
    {
        graph.setBounds (getLocalBounds().reduced (2));
    }
}

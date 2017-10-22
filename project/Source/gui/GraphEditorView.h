/*
    GraphEditorView.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "gui/ContentComponent.h"
#include "gui/GraphEditorComponent.h"

namespace Element {

class GraphEditorView : public ContentView
{
public:
    GraphEditorView();
    ~GraphEditorView();
    
    void didBecomeActive() override;
    void stabilizeContent() override;
    void paint (Graphics& g) override;
    void resized() override;
    
private:
    GraphEditorComponent graph;
};

}

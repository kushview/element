/*
    ContentComponent.cpp - This file is part of Element
    Copyright (C) 2015  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "gui/AssetTreeView.h"
#include "gui/GuiApp.h"
#include "gui/GraphEditorView.h"
#include "gui/ContentComponent.h"
#include "gui/NavigationView.h"
#include "gui/RackView.h"
#include "gui/TransportBar.h"
#include "session/Session.h"
#include "EngineControl.h"

namespace Element {

class ContentContainer : public Component
{
public:
    ContentContainer (GuiApp& gui)
    {
        AudioEnginePtr engine = gui.globals().engine();
        Shared<EngineControl> ctl = engine->controller();
        SessionRef session = gui.session();
        const AssetTree::Item root (session->assets().root());
        
        vertical = false;
        addAndMakeVisible (navView = new NavigationView());
        addAndMakeVisible (bar = new StretchableLayoutResizerBar (&layout, 1, ! vertical));
        addAndMakeVisible (graph = new GraphEditorView (gui, *ctl));
        updateLayout();
        resized();
    }
    
    virtual ~ContentContainer() { }
    
    bool isVertical() const { return vertical; }
    
    void resized() override
    {
        Component* comps[] = { navView.get(), bar.get(), graph.get() };
        layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), vertical, true);
    }
    
    void stabilize()
    {
        graph->resized();
        navView->resized();
    }
    
private:
    StretchableLayoutManager layout;
    ScopedPointer<NavigationView> navView;
    ScopedPointer<StretchableLayoutResizerBar> bar;
    ScopedPointer<GraphEditorView> graph;
    bool vertical;
    
    void updateLayout()
    {
        layout.setItemLayout (0, 200, 300, 200);
        layout.setItemLayout (1, 4, 4, 4);
        layout.setItemLayout (2, 0, -1.0, 500);
    }
};

ContentComponent::ContentComponent (GuiApp& app_)
    : gui (app_)
{
    AudioEnginePtr engine = gui.globals().engine();
    Shared<EngineControl> ctl = engine->controller();
    playbackMonitor = gui.session()->getPlaybackMonitor();
    
    setOpaque (true);
    
    addAndMakeVisible (bar1 = new StretchableLayoutResizerBar (&layoutVertical, 1, false));
    addAndMakeVisible (transport = new Element::TransportBar (gui.session()));
    addAndMakeVisible (rack = new RackView());
    addAndMakeVisible (top = new ContentContainer (gui));
    
    layoutVertical.setItemLayout (0, 300.0f, -1.0f, 500.0f);
    layoutVertical.setItemLayout (1, 4, 4, 4);
    layoutVertical.setItemLayout (2, 300.0f, 300.0f, 300.0f);
    
    startTimer (17);
    resized();
}

ContentComponent::~ContentComponent()
{
    toolTips = nullptr;
}

void ContentComponent::childBoundsChanged (Component* child)
{
}

void ContentComponent::paint (Graphics &g)
{
    g.fillAll (Colours::darkgrey);
}

void ContentComponent::resized()
{
    transport->setBounds (2, 2, transport->getWidth(), transport->getHeight());

    Component* comps[3] = { top.get(), bar1.get(), rack.get() };
    layoutVertical.layOutComponents (comps, 3,
                                     0,
                                     2 + transport->getHeight(),
                                     getWidth(),
                                     getHeight() - 2 + transport->getHeight(),
                                     true, true);
}

void ContentComponent::setRackViewComponent (Component* comp)
{
    rack->setMainComponent (comp);
}

void ContentComponent::setRackViewNode (GraphNodePtr node)
{
    jassert (node);
    auto* instance = node->getAudioPluginInstance();
    jassert (instance);
    const PluginDescription desc (instance->getPluginDescription());
    if (desc.pluginFormatName == "Internal")
        setRackViewComponent (instance->createEditorIfNeeded());
}

GuiApp& ContentComponent::app() { return gui; }

void ContentComponent::stabilize()
{
    if (top) top->stabilize();
    transport->stabilize();
}

void ContentComponent::timerCallback()
{
    transport->setBeatTime (playbackMonitor->get());
}
    
}



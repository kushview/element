/*
    ContentComponent.cpp - This file is part of Element
    Copyright (C) 2015-2017  Kushview, LLC.  All rights reserved.
*/

#include "controllers/AppController.h"
#include "engine/GraphProcessor.h"
#include "gui/AudioIOPanelView.h"
#include "gui/PluginsPanelView.h"
#include "gui/ConnectionGrid.h"
#include "gui/GuiApp.h"
#include "gui/LookAndFeel.h"

#include "gui/ContentComponent.h"
#include "Globals.h"

namespace Element {

class NavigationConcertinaPanel : public ConcertinaPanel {
public:
    NavigationConcertinaPanel (Globals& g)
        : globals(g),
          headerHeight (30),
          defaultPanelHeight (80)
    {
        setLookAndFeel (&lookAndFeel);
        updateContent();
    }
    
    ~NavigationConcertinaPanel()
    {
        setLookAndFeel (nullptr);
    }
    
    void clearPanels()
    {
        Array<Component*> comps;
        for (int i = 0; i < getNumPanels(); ++i)
            comps.add (getPanel (i));
        for (int i = 0; i < comps.size(); ++i)
        {
            removePanel (comps[i]);
            this->removePanel(0);
        }
        names.clear();
        comps.clear();
    }
    
    void updateContent()
    {
        clearPanels();
        
        names.add ("Audio");
        Component* c = new AudioIOPanelView();
        addPanel (-1, c, true);
        setPanelHeaderSize (c, headerHeight);
        setMaximumPanelSize (c, 160);
        setPanelSize (c, 60, false);
        
        names.add ("Plugins");
        c = new PluginsPanelView (globals.plugins());
        addPanel (-1, c, true);
        setPanelHeaderSize (c, headerHeight);
    }
    
    const StringArray& getNames() const { return names; }
    const int getHeaderHeight() const { return headerHeight; }
    void setHeaderHeight (const int newHeight)
    {
        jassert (newHeight > 0);
        headerHeight = newHeight;
        updateContent();
    }
    
private:
    typedef Element::LookAndFeel ELF;
    Globals& globals;
    StringArray names;
    int headerHeight;
    int defaultPanelHeight;
    
    class LookAndFeel : public Element::LookAndFeel
    {
    public:
        LookAndFeel() { }
        ~LookAndFeel() { }
        
        void drawConcertinaPanelHeader (Graphics& g, const Rectangle<int>& area,
                                        bool isMouseOver, bool isMouseDown,
                                        ConcertinaPanel& panel, Component& comp)
        {
            auto* p = dynamic_cast<NavigationConcertinaPanel*> (&panel);
            int i = p->getNumPanels();
            while (--i >= 0) {
                if (p->getPanel(i) == &comp)
                    break;
            }
            ELF::drawConcertinaPanelHeader (g, area, isMouseOver, isMouseDown, panel, comp);
            g.setColour (Colours::white);
            Rectangle<int> r (area.withTrimmedLeft (20));
            g.drawText (p->getNames()[i], 20, 0, r.getWidth(), r.getHeight(),
                        Justification::centredLeft);
        }
    } lookAndFeel;
};
    
class ContentContainer : public Component
{
public:
    ContentContainer (AppController& app, GuiApp& gui)
    {
        AudioEnginePtr e (app.getGlobals().engine());
        GraphProcessor& graph (e->graph());
        
        addAndMakeVisible (dummy1 = new ConnectionGrid());
        addAndMakeVisible (bar = new StretchableLayoutResizerBar (&layout, 1, false));
        addAndMakeVisible (dummy2 = new Component());
        
        const Node root (graph.getNodesModel());
        dummy1->setGraphNode (node);
        
        updateLayout();
        resized();
    }
    
    virtual ~ContentContainer() { }
    
    void resized() override
    {
        Component* comps[] = { dummy1.get(), bar.get(), dummy2.get() };
        layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), true, true);
    }
    
    void stabilize()
    {

    }
    
private:
    StretchableLayoutManager layout;
    ScopedPointer<StretchableLayoutResizerBar> bar;
    ScopedPointer<ConnectionGrid> dummy1;
    ScopedPointer<Component> dummy2;
    
    void updateLayout()
    {
        layout.setItemLayout (0, 200, -1.0, 200);
        layout.setItemLayout (1, 0, 0, 0);
        layout.setItemLayout (2, 0, -1.0, 0);
    }
};

ContentComponent::ContentComponent (AppController& ctl_, GuiApp& app_)
    : controller (ctl_),
      gui (app_)
{
    setOpaque (true);
    
    addAndMakeVisible (nav = new NavigationConcertinaPanel (gui.globals()));
    addAndMakeVisible (bar1 = new StretchableLayoutResizerBar (&layout, 1, true));
    addAndMakeVisible (container = new ContentContainer (controller, gui));
    
    updateLayout();
    resized();
}

ContentComponent::~ContentComponent()
{
    toolTips = nullptr;
}

Globals& ContentComponent::getGlobals() { return controller.getGlobals(); }
    
void ContentComponent::childBoundsChanged (Component* child)
{
}

void ContentComponent::mouseDown (const MouseEvent& ev)
{
    Component::mouseDown (ev);
}

void ContentComponent::paint (Graphics &g)
{
    g.fillAll (LookAndFeel::backgroundColor);
}

void ContentComponent::resized()
{
    Component* comps[3] = { nav.get(), bar1.get(), container.get() };
    layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), false, true);
}

void ContentComponent::setRackViewComponent (Component* comp)
{
    
}

void ContentComponent::setRackViewNode (GraphNodePtr node)
{
    
}

void ContentComponent::post (Message* message)
{
    controller.postMessage (message);
}
    
GuiApp& ContentComponent::app() { return gui; }

void ContentComponent::stabilize() { }

void ContentComponent::updateLayout()
{
    layout.setItemLayout (0, 220, 220, 220);
    layout.setItemLayout (1, 4, 4, 4);
    layout.setItemLayout (2, 300, -1, 400);
}

}



/*
    ContentComponent.cpp - This file is part of Element
    Copyright (C) 2015-2017  Kushview, LLC.  All rights reserved.
*/

#include "gui/LookAndFeel.h"
#include "gui/ConnectionGrid.h"
#include "gui/ContentComponent.h"

namespace Element {

class NavigationConcertinaPanel : public ConcertinaPanel {
public:
    NavigationConcertinaPanel()
        : headerHeight (30),
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
        names.add ("MIDI");
        names.add ("Presets");

        for (int i = 0; i < names.size(); ++i)
        {
            auto* c = new Component (names [i]);
            c->setSize (getWidth(), 160);
            addPanel (1, c, true);
            setPanelHeaderSize (c, headerHeight);
            setPanelSize (c, defaultPanelHeight, false);
        }
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
    ContentContainer (GuiApp& gui)
    {
        addAndMakeVisible (dummy1 = new ConnectionGrid());
        addAndMakeVisible (bar = new StretchableLayoutResizerBar (&layout, 1, false));
        addAndMakeVisible (dummy2 = new Component());
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
    ScopedPointer<Component> dummy1, dummy2;
    
    void updateLayout()
    {
        layout.setItemLayout (0, 200, -1.0, 200);
        layout.setItemLayout (1, 0, 0, 0);
        layout.setItemLayout (2, 0, -1.0, 0);
    }
};

ContentComponent::ContentComponent (GuiApp& app_)
    : gui (app_)
{
    setOpaque (true);
    
    addAndMakeVisible (nav = new NavigationConcertinaPanel());
    addAndMakeVisible (bar1 = new StretchableLayoutResizerBar (&layout, 1, true));
    addAndMakeVisible (container = new ContentContainer (app_));
    
    updateLayout();
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

GuiApp& ContentComponent::app() { return gui; }

void ContentComponent::stabilize() { }

void ContentComponent::updateLayout()
{
    layout.setItemLayout (0, 220, 220, 220);
    layout.setItemLayout (1, 4, 4, 4);
    layout.setItemLayout (2, 300, -1, 400);
}

}



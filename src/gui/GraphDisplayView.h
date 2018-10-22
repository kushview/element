#pragma once

#include "gui/GuiCommon.h"
#include "gui/ContentComponent.h"
#include "gui/BreadCrumbComponent.h"

namespace Element {

/** This is a simple container which displays a breadcrumb above a content area */
class GraphDisplayView : public ContentView,
                         public Button::Listener
{
public:
    GraphDisplayView()
    {
        addAndMakeVisible (breadcrumb);
        addAndMakeVisible (configButton);
        configButton.setButtonText ("*");
        configButton.addListener (this);
    }

    virtual ~GraphDisplayView()
    { 
        configButton.removeListener (this);
    }

    inline void buttonClicked (Button* b) override
    {
        auto* const world = ViewHelpers::getGlobals(this);
        if (! world) return;
        if (b == &configButton) {
            world->getCommandManager().invokeDirectly (Commands::showGraphConfig, true);
        }
    }

    inline void setBreadCrumbVisible (const bool isVisible)
    {
        if (isVisible != breadcrumb.isVisible())
        {
            breadcrumb.setVisible (isVisible);
            resized();
        }
    }

    inline void setConfigButtonVisible (const bool isVisible)
    {
        if (isVisible != configButton.isVisible())
        {
            configButton.setVisible (isVisible);
            resized();
        }
    }

    inline void setNode (const Node& n)
    {
        Node oldGraph = graph;
        Node oldNode  = node;

        graph = n.isGraph() ? n : n.getParentGraph();
        node  = n.isGraph() ? Node() : n;

       if (oldGraph != graph || oldNode != node)
       {
           if (node.isValid())
               breadcrumb.setNode (node);
           else if (graph.isValid())
               breadcrumb.setNode (graph);
           else
               breadcrumb.setNode (Node());
           
           graphNodeChanged (graph, node);
       }
    }

    inline void resized() override
    {
        auto r = getLocalBounds().reduced (2);
        if (breadcrumb.isVisible())
            breadcrumb.setBounds (r.removeFromTop (24));
        graphDisplayResized (r);

        const int configButtonSize = 14;
        configButton.setBounds (getWidth() - configButtonSize - 4, 4, 
                                configButtonSize, configButtonSize);
    }

    Node getGraph() const { return graph; }
    Node getNode()  const { return node; }

protected:
    virtual void graphDisplayResized (const Rectangle<int>& area) =0;
    virtual void graphNodeChanged (const Node&, const Node&) { }

private:
    Node graph, node;
    BreadCrumbComponent breadcrumb;
    SettingButton configButton;
};

}

#pragma once

#include "gui/GuiCommon.h"
#include "gui/ContentComponent.h"
#include "gui/BreadCrumbComponent.h"

namespace Element {

/** This is a simple container which displays a breadcrumb above a content area */
class GraphDisplayView : public ContentView
{
public:
    GraphDisplayView()
    { 
        addAndMakeVisible (breadcrumb);
    }

    virtual ~GraphDisplayView() { }

    inline void setBreadCrumbVisible (const bool isVisible)
    {
        if (isVisible != breadcrumb.isVisible())
        {
            breadcrumb.setVisible (isVisible);
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
    }

    Node getGraph() const { return graph; }
    Node getNode()  const { return node; }

protected:
    virtual void graphDisplayResized (const Rectangle<int>& area) =0;
    virtual void graphNodeChanged (const Node&, const Node&) { }

private:
    Node graph, node;
    BreadCrumbComponent breadcrumb;
};

}

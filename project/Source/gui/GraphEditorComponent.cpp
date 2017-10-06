/*
    GraphEditorComponent.cpp - This file is part of Element
    Copyright (C) 2016  Kushview, LLC.  All rights reserved.

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

#include "ElementApp.h"
#include "controllers/GraphController.h"
#include "gui/ContentComponent.h"
#include "gui/GraphEditorComponent.h"
#include "gui/PluginWindow.h"
#include "session/PluginManager.h"

namespace Element {
class PinComponent   : public Component,
                       public SettableTooltipClient
{
public:
    PinComponent (GraphController& graph_, const uint32 filterID_,
                  const uint32 index_, const bool isInput_,
                  const PortType type_)
        : filterID (filterID_),
          port (index_),
          type (type_),
          isInput (isInput_),
          graph (graph_)
    {
        if (const GraphNodePtr node = graph.getNodeForId (filterID_))
        {
            String tip;

          #if 0
            if (isInput)
                tip = bus.inputBuses.getReference(index_).name;
            else
                tip = bus.outputBuses.getReference(index_).name;
          #endif
            
            if (tip.isEmpty())
            {
                tip = (isInput ? "Input " : "Output ") + String (index_ + 1);
            }

            setTooltip (tip);
        }

        setSize (16, 16);
    }

    void paint (Graphics& g)
    {
        const float w = (float) getWidth();
        const float h = (float) getHeight();

        Path p;
        p.addEllipse (w * 0.25f, h * 0.25f, w * 0.5f, h * 0.5f);
        p.addRectangle (w * 0.4f, isInput ? (0.5f * h) : 0.0f, w * 0.2f, h * 0.5f);
        g.setColour (getColor());
        g.fillPath (p);
    }

    Colour getColor() const
    {
        switch (this->type)
        {
            case PortType::Audio: return Colours::lightgreen; break;
            case PortType::Control: return Colours::lightblue;  break;
            default: break;
        }
        
        return Colours::black;
    }

    void mouseDown (const MouseEvent& e)
    {
        getGraphPanel()->beginConnectorDrag (isInput ? 0 : filterID,
                                             port,
                                             isInput ? filterID : 0,
                                             port,
                                             e);
    }

    void mouseDrag (const MouseEvent& e)
    {
        getGraphPanel()->dragConnector (e);
    }

    void mouseUp (const MouseEvent& e)
    {
        getGraphPanel()->endDraggingConnector (e);
    }

    const uint32 filterID;
    const uint32 port;
    const PortType type;
    const bool   isInput;

private:
    GraphController& graph;

    GraphEditorComponent* getGraphPanel() const noexcept
    {
        return findParentComponentOfClass<GraphEditorComponent>();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PinComponent)
};

class FilterComponent    : public Component
{
public:
    FilterComponent (GraphController& graph_, const uint32 filterID_)
        : graph (graph_),
          filterID (filterID_),
          numInputs (0),
          numOutputs (0),
          pinSize (16),
          font (13.0f, Font::bold),
          numIns (0),
          numOuts (0)
    {
        shadow.setShadowProperties (DropShadow (Colours::black.withAlpha (0.5f), 3, Point<int> (0, 1)));
        setComponentEffect (&shadow);

        setSize (150, 60);
    }

    ~FilterComponent()
    {
        deleteAllChildren();
    }

    void mouseDown (const MouseEvent& e)
    {
        originalPos = localPointToGlobal (Point<int>());
        toFront (true);

        if (e.mods.isPopupMenu())
        {
            GraphNodePtr node = graph.getNodeForId (filterID);
            auto* instance = node->getAudioPluginInstance();
            const PluginDescription desc (instance->getPluginDescription());

            PopupMenu menu;
            menu.addItem (1, "Remove this block...");
            menu.addItem (2, "Disconnect all ports");
            menu.addSeparator();
            
            if (desc.pluginFormatName != "Internal")
            {
                menu.addItem (3, "Show plugin UI");
                menu.addItem (4, "Show generic UI");
            }
            
            const int r = menu.show();

            if (r == 1)
            {
                PluginWindow::closeCurrentlyOpenWindowsFor (filterID);
                
                // FIXME: Hack gui management: Fix in EL-54
                auto* cc = findParentComponentOfClass<ContentComponent>();
                if (GraphNodePtr gnp = graph.getNodeForId (filterID))
                    if (auto* i = gnp->getAudioPluginInstance())
                        if (auto* c = i->getActiveEditor())
                            cc->setRackViewComponent (c);
                embedded = nullptr;
                graph.removeFilter (filterID);
                return;
            }
            else if (r == 2)
            {
                graph.disconnectFilter (filterID);
            }
            else if (r == 3 || r == 4)
            {
                if (GraphNodePtr f = graph.getNodeForId (filterID))
                {
                    if (PluginWindow* win = getGraphPanel()->getOrCreateWindowForNode (f, r == 4))
                        win->toFront (true);
                }
            }
        }
    }

    void updatePosition()
    {
        double x = 0.0f, y = 0.0f;
//        graph.getNodePosition (filterID, x, y);
        setCentreRelative ((float) x, (float) y);
        getGraphPanel()->updateConnectorComponents();
    }
    
    void mouseDrag (const MouseEvent& e)
    {
        if (e.mods.isPopupMenu())
            return;
        
        Point<int> pos (originalPos + Point<int> (e.getDistanceFromDragStartX(), e.getDistanceFromDragStartY()));
        
        if (getParentComponent() != nullptr)
            pos = getParentComponent()->getLocalPoint (nullptr, pos);
        
//        graph.setNodePosition (filterID,
//                               (pos.getX() + getWidth() / 2) / (double) getParentWidth(),
//                               (pos.getY() + getHeight() / 2) / (double) getParentHeight());
//
        updatePosition();
    }

    void makeEditorActive()
    {
        const GraphNodePtr f = graph.getNodeForId (filterID);
        if (! f) return;
        
        auto* instance = f->getAudioPluginInstance();
        if (! instance) return;
        
        const PluginDescription desc (instance->getPluginDescription());
        
        if (desc.pluginFormatName != "Internal")
        {
            if (PluginWindow* const w = getGraphPanel()->getOrCreateWindowForNode (f, false))
                w->toFront (true);
        }
        else if (ScopedPointer<Component> ed = instance->createEditorIfNeeded())
        {
            if (ContentComponent* content = findParentComponentOfClass<ContentComponent>())
                content->setRackViewComponent (ed.release());
        }
    }
    
    void mouseUp (const MouseEvent& e)
    {
        if (e.mouseWasClicked() && e.getNumberOfClicks() == 2)
        {
            makeEditorActive();
        }
        else if (! e.mouseWasClicked())
        {
            graph.sendChangeMessage();
        }
    }

    bool hitTest (int x, int y)
    {
        for (int i = getNumChildComponents(); --i >= 0;)
            if (getChildComponent(i)->getBounds().contains (x, y))
                return true;

        return x >= 3 && x < getWidth() - 6 && y >= pinSize && y < getHeight() - pinSize;
    }

    void paint (Graphics& g)
    {
        g.setColour (Colours::lightgrey);

        const int x = 4;
        const int y = pinSize;
        const int w = getWidth() - x * 2;
        const int h = getHeight() - pinSize * 2;

        g.fillRect (x, y, w, h);

        g.setColour (Colours::black);
        g.setFont (font);
        g.drawFittedText (getName(), getLocalBounds().reduced (4, 2), Justification::centred, 2);

        g.setColour (Colours::grey);
        g.drawRect (x, y, w, h);
    }

    void resized()
    {
        int indexIn = 0, indexOut = 0;
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (PinComponent* const pc = dynamic_cast <PinComponent*> (getChildComponent(i)))
            {
                const int total = pc->isInput ? numIns : numOuts;
                const int index = pc->isInput ? indexIn++ : indexOut++;
                pc->setBounds (proportionOfWidth ((1 + index) / (total + 1.0f)) - pinSize / 2,
                               pc->isInput ? 0 : (getHeight() - pinSize),
                               pinSize, pinSize);
            }
        }
    }

    void getPinPos (const int index, const bool isInput, float& x, float& y)
    {
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (PinComponent* const pc = dynamic_cast <PinComponent*> (getChildComponent(i)))
            {
                if (pc->port == index && isInput == pc->isInput)
                {
                    x = getX() + pc->getX() + pc->getWidth() * 0.5f;
                    y = getY() + pc->getY() + pc->getHeight() * 0.5f;
                    break;
                }
            }
        }
    }

    void update()
    {
        const GraphNodePtr f (graph.getNodeForId (filterID));

        if (f == nullptr)
        {
            delete this;
            return;
        }

        numIns = numOuts = 0;
        for (uint32 i = 0; i < f->getProcessor()->getNumPorts(); ++i)
        {
            if (PortType::Control == f->getProcessor()->getPortType(i))
                continue;
            
            if (f->getProcessor()->isPortInput (i))
                ++numIns;
            else
                ++numOuts;
        }

        int w = 100;
        int h = 60;

        w = jmax (w, (jmax (numIns, numOuts) + 1) * 20);

        const int textWidth = font.getStringWidth (f->getProcessor()->getName());
        w = jmax (w, 16 + jmin (textWidth, 300));
        if (textWidth > 300)
            h = 100;

        setSize (w, h);

        setName (f->getProcessor()->getName());

        {
            double x, y;
            x = y = 0.0f;
//            graph.getNodePosition (filterID, x, y);
            setCentreRelative ((float) x, (float) y);
        }

        if (numIns != numInputs || numOuts != numOutputs)
        {
            numInputs = numIns;
            numOutputs = numOuts;

            deleteAllChildren();

            for (uint32 i = 0; i < f->getProcessor()->getNumPorts(); ++i)
            {
                const PortType t (f->getProcessor()->getPortType (i));
                if (t ==PortType::Control)
                    continue;
                
                const bool isInput (f->getProcessor()->isPortInput (i));
                addAndMakeVisible (new PinComponent (graph, filterID, i, isInput, t));
            }

            resized();
        }
    }

    GraphController& graph;
    const uint32 filterID;
    int numInputs, numOutputs;

private:
    int pinSize;
    Point<int> originalPos;
    Font font;
    int numIns, numOuts;
    DropShadowEffect shadow;
    ScopedPointer<Component> embedded;


    GraphEditorComponent* getGraphPanel() const noexcept
    {
        return findParentComponentOfClass<GraphEditorComponent>();
    }

    FilterComponent (const FilterComponent&);
    FilterComponent& operator= (const FilterComponent&);
};

class ConnectorComponent   : public Component,
                             public SettableTooltipClient
{
public:
    ConnectorComponent (GraphController& graph_)
        : sourceFilterID (0),
          destFilterID (0),
          sourceFilterChannel (0),
          destFilterChannel (0),
          graph (graph_),
          lastInputX (0),
          lastInputY (0),
          lastOutputX (0),
          lastOutputY (0)
    {
        setAlwaysOnTop (true);
    }

    ~ConnectorComponent() { }

    bool isDragging() const { return dragging; }

    void setInput (const uint32 sourceFilterID_, const int sourceFilterChannel_)
    {
        if (sourceFilterID != sourceFilterID_ || sourceFilterChannel != sourceFilterChannel_)
        {
            sourceFilterID = sourceFilterID_;
            sourceFilterChannel = sourceFilterChannel_;
            update();
        }
    }

    void setOutput (const uint32 destFilterID_, const int destFilterChannel_)
    {
        if (destFilterID != destFilterID_ || destFilterChannel != destFilterChannel_)
        {
            destFilterID = destFilterID_;
            destFilterChannel = destFilterChannel_;
            update();
        }
    }

    void dragStart (int x, int y)
    {
        lastInputX = (float) x;
        lastInputY = (float) y;
        resizeToFit();
    }

    void dragEnd (int x, int y)
    {
        lastOutputX = (float) x;
        lastOutputY = (float) y;
        resizeToFit();
    }

    void update()
    {
        float x1, y1, x2, y2;
        getPoints (x1, y1, x2, y2);

        if (lastInputX != x1
             || lastInputY != y1
             || lastOutputX != x2
             || lastOutputY != y2)
        {
            resizeToFit();
        }
    }

    void resizeToFit()
    {
        float x1, y1, x2, y2;
        getPoints (x1, y1, x2, y2);

        const Rectangle<int> newBounds ((int) jmin (x1, x2) - 4,
                                        (int) jmin (y1, y2) - 4,
                                        (int) fabsf (x1 - x2) + 8,
                                        (int) fabsf (y1 - y2) + 8);

        setBounds (newBounds);
        repaint();
    }

    void getPoints (float& x1, float& y1, float& x2, float& y2) const
    {
        x1 = lastInputX;
        y1 = lastInputY;
        x2 = lastOutputX;
        y2 = lastOutputY;

        if (GraphEditorComponent* const hostPanel = getGraphPanel())
        {
            if (FilterComponent* srcFilterComp = hostPanel->getComponentForFilter (sourceFilterID))
                srcFilterComp->getPinPos (sourceFilterChannel, false, x1, y1);

            if (FilterComponent* dstFilterComp = hostPanel->getComponentForFilter (destFilterID))
                dstFilterComp->getPinPos (destFilterChannel, true, x2, y2);
        }
    }

    void paint (Graphics& g)
    {
        g.setColour (Colours::black.brighter());
        g.fillPath (linePath);
    }

    bool hitTest (int x, int y)
    {
        if (hitPath.contains ((float) x, (float) y))
        {
            double distanceFromStart, distanceFromEnd;
            getDistancesFromEnds (x, y, distanceFromStart, distanceFromEnd);

            // avoid clicking the connector when over a pin
            return distanceFromStart > 7.0 && distanceFromEnd > 7.0;
        }

        return false;
    }

    void mouseDown (const MouseEvent&)
    {
        dragging = false;
    }

    void mouseDrag (const MouseEvent& e)
    {
        if ((! dragging) && ! e.mouseWasClicked())
        {
            dragging = true;

            graph.removeConnection (sourceFilterID, sourceFilterChannel, destFilterID, destFilterChannel);

            double distanceFromStart, distanceFromEnd;
            getDistancesFromEnds (e.x, e.y, distanceFromStart, distanceFromEnd);
            const bool isNearerSource = (distanceFromStart < distanceFromEnd);

            getGraphPanel()->beginConnectorDrag (isNearerSource ? 0 : sourceFilterID,
                                                 sourceFilterChannel,
                                                 isNearerSource ? destFilterID : 0,
                                                 destFilterChannel,
                                                 e);

        }
        else if (dragging)
        {
            getGraphPanel()->dragConnector (e);
        }
    }

    void mouseUp (const MouseEvent& e)
    {
        if (dragging)
            getGraphPanel()->endDraggingConnector (e);
    }

    void resized()
    {
        float x1, y1, x2, y2;
        getPoints (x1, y1, x2, y2);

        lastInputX = x1;
        lastInputY = y1;
        lastOutputX = x2;
        lastOutputY = y2;

        x1 -= getX();
        y1 -= getY();
        x2 -= getX();
        y2 -= getY();

        linePath.clear();
        linePath.startNewSubPath (x1, y1);
        linePath.cubicTo (x1, y1 + (y2 - y1) * 0.33f,
                          x2, y1 + (y2 - y1) * 0.66f,
                          x2, y2);

        PathStrokeType wideStroke (8.0f);
        wideStroke.createStrokedPath (hitPath, linePath);

        PathStrokeType stroke (2.5f);
        stroke.createStrokedPath (linePath, linePath);

        const float arrowW = 5.0f;
        const float arrowL = 4.0f;

        Path arrow;
        arrow.addTriangle (-arrowL, arrowW,
                           -arrowL, -arrowW,
                           arrowL, 0.0f);

        arrow.applyTransform (AffineTransform::identity
                                .rotated (float_Pi * 0.5f - (float) atan2 (x2 - x1, y2 - y1))
                                .translated ((x1 + x2) * 0.5f,
                                             (y1 + y2) * 0.5f));

        linePath.addPath (arrow);
        linePath.setUsingNonZeroWinding (true);
    }

    uint32 sourceFilterID, destFilterID;
    int sourceFilterChannel, destFilterChannel;

private:
    GraphController& graph;
    float lastInputX, lastInputY, lastOutputX, lastOutputY;
    Path linePath, hitPath;
    bool dragging;

    GraphEditorComponent* getGraphPanel() const noexcept
    {
        return findParentComponentOfClass<GraphEditorComponent>();
    }

    void getDistancesFromEnds (int x, int y, double& distanceFromStart, double& distanceFromEnd) const
    {
        float x1, y1, x2, y2;
        getPoints (x1, y1, x2, y2);

        distanceFromStart = juce_hypot (x - (x1 - getX()), y - (y1 - getY()));
        distanceFromEnd = juce_hypot (x - (x2 - getX()), y - (y2 - getY()));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectorComponent)
};

GraphEditorComponent::GraphEditorComponent (GraphController& graph_)
    :  graph (graph_)
{
    graph.addChangeListener (this);
    setOpaque (true);
}

GraphEditorComponent::~GraphEditorComponent()
{
    graph.removeChangeListener (this);
    draggingConnector = nullptr;
    deleteAllChildren();
}

void GraphEditorComponent::paint (Graphics& g)
{
    g.fillAll (Colour (0xff303030));
}

void GraphEditorComponent::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        PopupMenu m;
        m.addSectionHeader ("Plugins");
        KnownPluginList& plugs (graph.plugins().availablePlugins());
        plugs.addToMenu (m, KnownPluginList::sortByManufacturer);
        
        const int res = m.show();
        
        if (const PluginDescription* desc = plugs.getType (plugs.getIndexChosenByMenu (res)))
            createNewPlugin (desc, e.x, e.y);
    }
}

void GraphEditorComponent::createNewPlugin (const PluginDescription* desc, int x, int y)
{
    graph.addFilter (desc, x / (double) getWidth(), y / (double) getHeight());
}

FilterComponent* GraphEditorComponent::getComponentForFilter (const uint32 filterID) const
{
    for (int i = getNumChildComponents(); --i >= 0;)
    {
        if (FilterComponent* const fc = dynamic_cast <FilterComponent*> (getChildComponent (i)))
            if (fc->filterID == filterID)
                return fc;
    }

    return nullptr;
}

ConnectorComponent* GraphEditorComponent::getComponentForConnection (const Arc& arc) const
{
    for (int i = getNumChildComponents(); --i >= 0;)
    {
        if (ConnectorComponent* const c = dynamic_cast <ConnectorComponent*> (getChildComponent (i)))
            if (c->sourceFilterID == arc.sourceNode
                 && c->destFilterID == arc.destNode
                 && c->sourceFilterChannel == arc.sourcePort
                 && c->destFilterChannel == arc.destPort)
                return c;
    }

    return nullptr;
}

PinComponent* GraphEditorComponent::findPinAt (const int x, const int y) const
{
    for (int i = getNumChildComponents(); --i >= 0;)
    {
        if (FilterComponent* fc = dynamic_cast <FilterComponent*> (getChildComponent (i)))
        {
            if (PinComponent* pin = dynamic_cast <PinComponent*> (fc->getComponentAt (x - fc->getX(),
                                                                                      y - fc->getY())))
                return pin;
        }
    }

    return nullptr;
}

void GraphEditorComponent::resized()
{
    updateComponents();
}

void GraphEditorComponent::changeListenerCallback (ChangeBroadcaster*)
{
    updateComponents();
}

void GraphEditorComponent::onGraphChanged()
{
    updateComponents();
}

void GraphEditorComponent::updateConnectorComponents()
{
    for (int i = getNumChildComponents(); --i >= 0;)
    {
        ConnectorComponent* const cc = dynamic_cast <ConnectorComponent*> (getChildComponent (i));
        
        if (cc != nullptr && cc != draggingConnector)
        {
            if (graph.getConnectionBetween (cc->sourceFilterID, cc->sourceFilterChannel,
                                            cc->destFilterID, cc->destFilterChannel) == nullptr)
            {
                delete cc;
            }
            else
            {
                cc->update();
            }
        }
    }
}
    
void GraphEditorComponent::updateComponents()
{
    for (int i = graph.getNumConnections(); --i >= 0;)
    {
        const GraphProcessor::Connection* const c = graph.getConnection (i);
        ConnectorComponent* connector = getComponentForConnection (*c);
        
        if (connector == nullptr)
        {
            connector = new ConnectorComponent (graph);
            addAndMakeVisible (connector);
        }
        
        connector->setInput (c->sourceNode, c->sourcePort);
        connector->setOutput (c->destNode, c->destPort);
        connector->update();
    }
    
    for (int i = graph.getNumFilters(); --i >= 0;)
    {
        const GraphNodePtr f (graph.getNode (i));
        FilterComponent* comp = getComponentForFilter (f->nodeId);
        if (comp == nullptr)
        {
            comp = new FilterComponent (graph, f->nodeId);
            addAndMakeVisible (comp);
        }
        
        comp->update();
    }
    
    for (int i = getNumChildComponents(); --i >= 0;)
    {
        if (FilterComponent* const fc = dynamic_cast <FilterComponent*> (getChildComponent (i)))
            fc->update();
    }

    for (int i = getNumChildComponents(); --i >= 0;)
    {
        ConnectorComponent* const cc = dynamic_cast <ConnectorComponent*> (getChildComponent (i));

        if (cc != nullptr && cc != draggingConnector && ! cc->isDragging())
        {
            if (graph.getConnectionBetween (cc->sourceFilterID, cc->sourceFilterChannel,
                                            cc->destFilterID, cc->destFilterChannel) == nullptr)
            {
                delete cc;
            }
            else
            {
                cc->update();
            }
        }
    }
}

void GraphEditorComponent::beginConnectorDrag (const uint32 sourceFilterID, const int sourceFilterChannel,
                                          const uint32 destFilterID, const int destFilterChannel,
                                          const MouseEvent& e)
{
    draggingConnector = dynamic_cast <ConnectorComponent*> (e.originalComponent);
    if (draggingConnector == nullptr)
        draggingConnector = new ConnectorComponent (graph);

    draggingConnector->setInput (sourceFilterID, sourceFilterChannel);
    draggingConnector->setOutput (destFilterID, destFilterChannel);

    addAndMakeVisible (draggingConnector);
    draggingConnector->toFront (false);

    dragConnector (e);
}

void GraphEditorComponent::dragConnector (const MouseEvent& e)
{
    const MouseEvent e2 (e.getEventRelativeTo (this));

    if (draggingConnector != nullptr)
    {
        draggingConnector->setTooltip (String::empty);

        int x = e2.x;
        int y = e2.y;

        if (PinComponent* const pin = findPinAt (x, y))
        {
            uint32 srcFilter = draggingConnector->sourceFilterID;
            int srcChannel   = draggingConnector->sourceFilterChannel;
            uint32 dstFilter = draggingConnector->destFilterID;
            int dstChannel   = draggingConnector->destFilterChannel;

            if (srcFilter == 0 && ! pin->isInput)
            {
                srcFilter = pin->filterID;
                srcChannel = pin->port;
            }
            else if (dstFilter == 0 && pin->isInput)
            {
                dstFilter = pin->filterID;
                dstChannel = pin->port;
            }

            if (graph.canConnect (srcFilter, srcChannel, dstFilter, dstChannel))
            {
                x = pin->getParentComponent()->getX() + pin->getX() + pin->getWidth() / 2;
                y = pin->getParentComponent()->getY() + pin->getY() + pin->getHeight() / 2;

                draggingConnector->setTooltip (pin->getTooltip());
            }
        }

        if (draggingConnector->sourceFilterID == 0)
            draggingConnector->dragStart (x, y);
        else
            draggingConnector->dragEnd (x, y);
    }
}

Component* GraphEditorComponent::createContainerForNode (GraphNodePtr node, bool useGenericEditor)
{
    if (AudioProcessorEditor* ed = createEditorForNode (node, useGenericEditor))
        if (Component* comp = wrapAudioProcessorEditor (ed, node))
            return comp;
    return nullptr;
}

Component* GraphEditorComponent::wrapAudioProcessorEditor(AudioProcessorEditor* ed, GraphNodePtr) { return ed; }

AudioProcessorEditor* GraphEditorComponent::createEditorForNode (GraphNodePtr node, bool useGenericEditor)
{
    ScopedPointer<AudioProcessorEditor> ui = nullptr;
    
    if (! useGenericEditor)
    {
        ui = node->getProcessor()->createEditorIfNeeded();
        
        if (ui == nullptr)
            useGenericEditor = true;
    }
    
    if (useGenericEditor)
        ui = new GenericAudioProcessorEditor (node->getProcessor());
    
    return (nullptr != ui) ? ui.release() : nullptr;
}

void GraphEditorComponent::endDraggingConnector (const MouseEvent& e)
{
    if (draggingConnector == nullptr)
        return;

    draggingConnector->setTooltip (String::empty);

    const MouseEvent e2 (e.getEventRelativeTo (this));

    uint32 srcFilter = draggingConnector->sourceFilterID;
    int srcChannel   = draggingConnector->sourceFilterChannel;
    uint32 dstFilter = draggingConnector->destFilterID;
    int dstChannel   = draggingConnector->destFilterChannel;

    draggingConnector = nullptr;

    if (PinComponent* const pin = findPinAt (e2.x, e2.y))
    {
        if (srcFilter == 0)
        {
            if (pin->isInput)
                return;

            srcFilter = pin->filterID;
            srcChannel = pin->port;
        }
        else
        {
            if (! pin->isInput)
                return;

            dstFilter = pin->filterID;
            dstChannel = pin->port;
        }

        graph.addConnection (srcFilter, srcChannel, dstFilter, dstChannel);
    }
}

PluginWindow* GraphEditorComponent::getOrCreateWindowForNode (GraphNodePtr f, bool useGeneric)
{
    if (PluginWindow* window = PluginWindow::getWindowFor (f))
        return window;
    
    PluginWindow* w = PluginWindow::getFirstWindow();
    ScopedPointer<Component> c (createContainerForNode (f, useGeneric));
    
    if (! w && c) {
            w = PluginWindow::createWindowFor (f, c.get());
            if (w)
                c.release();
    }
    else if (w && c)
    {
        w->updateGraphNode (f, c.release());
    }

    return w;
}


bool GraphEditorComponent::isInterestedInDragSource (const SourceDetails& details)
{
    if (! details.description.isArray())
        return false;
    
    if (auto* a = details.description.getArray())
    {
        const var type (a->getFirst());
        return type == var ("element://dnd/plugin");
    }
    
    return false;
}

void GraphEditorComponent::itemDropped (const SourceDetails& details)
{
    if (const auto* a = details.description.getArray())
    {
        PluginDescription desc;
        desc.pluginFormatName = a->getUnchecked (1);
        desc.fileOrIdentifier = a->getUnchecked (2);
        createNewPlugin (&desc, details.localPosition.x, details.localPosition.y);
    }
}

bool shouldDrawDragImageWhenOver()
{
    return true;
}


class TooltipBar   : public Component,
                     private Timer
{
public:
    TooltipBar()
    {
        startTimer (100);
    }

    void paint (Graphics& g)
    {
        g.setFont (Font (getHeight() * 0.7f, Font::bold));
        g.setColour (Colours::black);
        g.drawFittedText (tip, 10, 0, getWidth() - 12, getHeight(), Justification::centredLeft, 1);
    }

    void timerCallback()
    {
        Component* const underMouse = Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();
        TooltipClient* const ttc = dynamic_cast <TooltipClient*> (underMouse);

        String newTip;

        if (ttc != nullptr && ! (underMouse->isMouseButtonDown() || underMouse->isCurrentlyBlockedByAnotherModalComponent()))
            newTip = ttc->getTooltip();

        if (newTip != tip)
        {
            tip = newTip;
            repaint();
        }
    }

private:
    String tip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TooltipBar)
};

}

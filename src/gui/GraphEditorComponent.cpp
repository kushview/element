/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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
#include "controllers/GraphManager.h"

#include "gui/GuiCommon.h"
#include "gui/BlockComponent.h"
#include "gui/ContentComponent.h"
#include "gui/ContextMenus.h"
#include "gui/Icons.h"
#include "gui/PluginWindow.h"

#include "gui/AudioIOPanelView.h"
#include "gui/SessionTreePanel.h"
#include "gui/views/PluginsPanelView.h"
#include "gui/NavigationConcertinaPanel.h"
#include "gui/NodeIOConfiguration.h"
#include "engine/nodes/BaseProcessor.h"

#include "session/PluginManager.h"
#include "session/Presets.h"
#include "session/Node.h"

#include "gui/GraphEditorComponent.h"

#include "ScopedFlag.h"

namespace Element {

static bool elNodeIsAudioMixer (const Node& node)
{
    return node.getFormat().toString() == "Element"
           && node.getIdentifier().toString() == "element.audioMixer";
}

static bool elNodeIsMidiDevice (const Node& node)
{
    return node.getFormat().toString() == "Internal"
           && (node.getIdentifier().toString() == "element.midiInputDevice" || node.getIdentifier().toString() == "element.midiOutputDevice");
}

static bool elNodeCanChangeIO (const Node& node)
{
    return ! node.isIONode()
           && ! node.isGraph()
           && ! elNodeIsAudioMixer (node)
           && ! elNodeIsMidiDevice (node);
}

class DefaultBlockFactory : public BlockFactory
{
public:
    DefaultBlockFactory (GraphEditorComponent& e)
        : editor (e) {}

    BlockComponent* createBlockComponent (AppController& app, const Node& node) override
    {
        ignoreUnused (app);
        auto* const block = new BlockComponent (node.getParentGraph(), node, editor.isLayoutVertical());

        if (node.isIONode() || node.isRootGraph())
        {
            block->setMuteButtonVisible (false);
            block->setPowerButtonVisible (false);
        }

        if (! elNodeCanChangeIO (node))
        {
            block->setConfigButtonVisible (false);
        }

        return block;
    }

private:
    GraphEditorComponent& editor;
};

//=============================================================================
class ConnectorComponent : public Component,
                           public SettableTooltipClient
{
public:
    ConnectorComponent (const Node& g)
        : sourceFilterID (0), destFilterID (0), sourceFilterChannel (0), destFilterChannel (0), graph (g), lastInputX (0), lastInputY (0), lastOutputX (0), lastOutputY (0)
    {
    }

    ~ConnectorComponent() {}

    bool isDragging() const { return dragging; }
    void setGraph (const Node& g) { graph = g; }

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

    bool getPoints (float& x1, float& y1, float& x2, float& y2) const
    {
        bool sres = false, dres = false;

        x1 = lastInputX;
        y1 = lastInputY;
        x2 = lastOutputX;
        y2 = lastOutputY;

        if (GraphEditorComponent* const hostPanel = getGraphPanel())
        {
            if (auto* srcBlock = hostPanel->getComponentForFilter (sourceFilterID))
                sres = srcBlock->getPortPos (sourceFilterChannel, false, x1, y1);

            if (auto* dstBlock = hostPanel->getComponentForFilter (destFilterID))
                dres = dstBlock->getPortPos (destFilterChannel, true, x2, y2);
        }

        return sres && dres;
    }

    void paint (Graphics& g) override
    {
        auto c = Colours::black.brighter();
        if (hover || dragging)
            c = c.brighter (0.2);
        g.setColour (c);
        g.fillPath (linePath);
    }

    bool hitTest (int x, int y) override
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

    void mouseEnter (const MouseEvent&) override
    {
        if (hover)
            return;
        hover = true;
        repaint();
    }

    void mouseExit (const MouseEvent&) override
    {
        if (! hover)
            return;
        hover = false;
        repaint();
    }

    void mouseDown (const MouseEvent&) override
    {
        if (! isEnabled())
            return;
        dragging = false;
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (! isEnabled())
            return;

        if ((! dragging) && ! e.mouseWasClicked())
        {
            dragging = true;
            repaint();

            double distanceFromStart, distanceFromEnd;
            getDistancesFromEnds (e.x, e.y, distanceFromStart, distanceFromEnd);
            const bool isNearerSource = (distanceFromStart < distanceFromEnd);
            ViewHelpers::postMessageFor (this, new RemoveConnectionMessage (sourceFilterID, (uint32) sourceFilterChannel, destFilterID, (uint32) destFilterChannel, graph));

            getGraphPanel()->beginConnectorDrag (isNearerSource ? 0 : sourceFilterID, sourceFilterChannel, isNearerSource ? destFilterID : 0, destFilterChannel, e);
        }
        else if (dragging)
        {
            getGraphPanel()->dragConnector (e);
        }
    }

    void mouseUp (const MouseEvent& e) override
    {
        if (! isEnabled())
            return;
        if (dragging)
            getGraphPanel()->endDraggingConnector (e);
    }

    void resized() override
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
        const bool vertical = getGraphPanel()->isLayoutVertical();

        if (vertical)
        {
            linePath.cubicTo (x1, y1 + (y2 - y1) * 0.33f, x2, y1 + (y2 - y1) * 0.66f, x2, y2);
        }
        else
        {
            linePath.cubicTo (x1 + (x2 - x1) * 0.33f, y1, x1 + (x2 - x1) * 0.66f, y2, x2, y2);
        }

        PathStrokeType wideStroke (8.0f);
        wideStroke.createStrokedPath (hitPath, linePath);

        PathStrokeType stroke (2.5f);
        stroke.createStrokedPath (linePath, linePath);

        const bool showArrow = false;

        if (showArrow)
        {
            const float arrowW = 5.0f;
            const float arrowL = 4.0f;

            Path arrow;
            arrow.addTriangle (-arrowL, arrowW, -arrowL, -arrowW, arrowL, 0.0f);

            arrow.applyTransform (AffineTransform()
                                      .rotated (float_Pi * 0.5f - (float) atan2 (x2 - x1, y2 - y1))
                                      .translated ((x1 + x2) * 0.5f,
                                                   (y1 + y2) * 0.5f));

            linePath.addPath (arrow);
        }

        linePath.setUsingNonZeroWinding (true);
    }

    uint32 sourceFilterID { KV_INVALID_PORT },
        destFilterID { KV_INVALID_PORT };
    int sourceFilterChannel, destFilterChannel;

private:
    Node graph;
    float lastInputX, lastInputY, lastOutputX, lastOutputY;
    Path linePath, hitPath;
    bool dragging { false };
    bool hover { false };

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

//=============================================================================
void GraphEditorComponent::SelectedNodes::itemSelected (uint32 nodeId)
{
    for (int i = 0; i < editor.getNumChildComponents(); ++i)
        if (auto* block = dynamic_cast<BlockComponent*> (editor.getChildComponent (i)))
            if (nodeId == block->node.getNodeId())
                block->setSelectedInternal (true);
}

void GraphEditorComponent::SelectedNodes::itemDeselected (uint32 nodeId)
{
    for (int i = 0; i < editor.getNumChildComponents(); ++i)
        if (auto* block = dynamic_cast<BlockComponent*> (editor.getChildComponent (i)))
            if (nodeId == block->node.getNodeId())
                block->setSelectedInternal (false);
}

//=============================================================================
GraphEditorComponent::GraphEditorComponent()
    : ViewHelperMixin (this),
      selectedNodes (*this)
{
    factory.reset (new DefaultBlockFactory (*this));
    setOpaque (true);
    data.addListener (this);
}

GraphEditorComponent::~GraphEditorComponent()
{
    data.removeListener (this);
    graph = Node();
    data = ValueTree();
    draggingConnector = nullptr;
    resizePositionsFrozen = false;
    deleteAllChildren();

    factory.reset();
}

void GraphEditorComponent::setNode (const Node& n)
{
    if (n == graph)
        return;
    bool isGraph = n.isGraph();
    bool isValid = n.isValid();
    graph = isValid && isGraph ? n : Node (Tags::graph);

    data.removeListener (this);
    data = graph.getValueTree();

    verticalLayout = graph.getProperty (Tags::vertical, true);
    resizePositionsFrozen = (bool) graph.getProperty (Tags::staticPos, false);

    if (draggingConnector)
        removeChildComponent (draggingConnector.get());
    deleteAllChildren();
    updateComponents();
    if (draggingConnector)
        addAndMakeVisible (draggingConnector.get());

    data.addListener (this);
}

void GraphEditorComponent::setVerticalLayout (const bool isVertical)
{
    if (verticalLayout == isVertical)
        return;
    verticalLayout = isVertical;

    if (graph.isValid() && graph.isGraph())
        graph.setProperty ("vertical", verticalLayout);

    draggingConnector = nullptr;
    deleteAllChildren();
    updateComponents();
}

void GraphEditorComponent::paint (Graphics& g)
{
    g.fillAll (findColour (Style::contentBackgroundColorId));
}

void GraphEditorComponent::mouseDown (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    lastDropX = (float) e.getMouseDownX() / (float) getWidth();
    lastDropY = (float) e.getMouseDownY() / (float) getHeight();

    if (selectedNodes.getNumSelected() > 0)
    {
        selectedNodes.deselectAll();
        updateSelection();
    }

    if (e.mods.isPopupMenu())
    {
        PluginsPopupMenu menu (this);
        if (graph.isGraph())
        {
            menu.addSectionHeader ("Graph I/O");
            menu.addItem (1, "Audio Inputs", true, graph.hasAudioInputNode());
            menu.addItem (2, "Audio Outputs", true, graph.hasAudioOutputNode());
            menu.addItem (3, "MIDI Input", true, graph.hasMidiInputNode());
            menu.addItem (4, "MIDI Output", true, graph.hasMidiOutputNode());
            menu.addSeparator();

            PopupMenu submenu;
            addMidiDevicesToMenu (submenu, true, 80000);
            menu.addSubMenu ("MIDI Input Device", submenu);
            submenu.clear();
            addMidiDevicesToMenu (submenu, false, 90000);
            menu.addSubMenu ("MIDI Output Device", submenu);
        }

        menu.addSeparator();
        PopupMenu zoomMenu;
        // zoomMenu.addItem (50, "25%",  true, getZoomScale() == 0.25);
        // zoomMenu.addItem (51, "50%",  true, getZoomScale() == 0.50);
        zoomMenu.addItem (52, "75%", true, getZoomScale() == 0.75);
        zoomMenu.addItem (53, "100%", true, getZoomScale() == 1.00);
        zoomMenu.addItem (54, "125%", true, getZoomScale() == 1.25);
        zoomMenu.addItem (55, "150%", true, getZoomScale() == 1.50);
        zoomMenu.addItem (56, "175%", true, getZoomScale() == 1.75);
        zoomMenu.addItem (57, "200%", true, getZoomScale() == 2.00);
        menu.addSubMenu ("Zoom", zoomMenu);

        menu.addItem (5, "Change orientation...");
        menu.addItem (7, "Gather nodes...");
#if JUCE_DEBUG
        menu.addItem (100, "Misc testing item...");
#endif
        menu.addSeparator();

        menu.addSectionHeader ("Plugins");
        menu.addPluginItems();
        const int result = menu.show();

        if (menu.isPluginResultCode (result))
        {
            bool verified = false;
            const auto desc = menu.getPluginDescription (result, verified);
            if (desc.fileOrIdentifier.isNotEmpty() && desc.pluginFormatName.isNotEmpty())
                ViewHelpers::postMessageFor (this, new AddPluginMessage (graph, desc, verified));
        }
        else if (result >= 80000 && result < 90000)
        {
            ViewHelpers::postMessageFor (this,
                                         new AddMidiDeviceMessage (getMidiDeviceForMenuResult (result, true), true));
        }
        else if (result >= 90000 && result < 100000)
        {
            ViewHelpers::postMessageFor (this,
                                         new AddMidiDeviceMessage (getMidiDeviceForMenuResult (result, false, 90000), false));
        }
        else
        {
            PluginDescription desc;
            desc.pluginFormatName = "Internal";
            bool hasRequestedType = false;
            bool failure = false;

            switch (result)
            {
                case 1:
                    desc.fileOrIdentifier = "audio.input";
                    hasRequestedType = graph.hasAudioInputNode();
                    break;
                case 2:
                    desc.fileOrIdentifier = "audio.output";
                    hasRequestedType = graph.hasAudioOutputNode();
                    break;
                case 3:
                    desc.fileOrIdentifier = "midi.input";
                    hasRequestedType = graph.hasMidiInputNode();
                    break;
                case 4:
                    desc.fileOrIdentifier = "midi.output";
                    hasRequestedType = graph.hasMidiOutputNode();
                    break;
                case 5:
                    setVerticalLayout (! isLayoutVertical());
                    return;
                    break;

                case 7: {
                    int width = getWidth();
                    int height = getHeight();
                    int numChanges = 0;
                    int numEditorChanges = 0;
                    for (int i = 0; i < getNumChildComponents(); ++i)
                    {
                        auto* const block = dynamic_cast<BlockComponent*> (getChildComponent (i));
                        if (nullptr == block)
                            continue;

                        auto r = block->getBounds();
                        // auto x = r.getX(), y = r.getY();
                        // if (! isLayoutVertical())
                        bool changed = false;
                        if (r.getX() < 0)
                        {
                            changed = true;
                            r = r.withX (0);
                        }
                        else if (r.getRight() > width)
                        {
                            numEditorChanges++;
                            width = r.getRight();
                        }

                        if (r.getY() < 0)
                        {
                            changed = true;
                            r = r.withY (0);
                        }
                        else if (r.getBottom() > height)
                        {
                            numEditorChanges++;
                            height = r.getBottom();
                        }

                        if (changed)
                        {
                            block->moveBlockTo (r.getX(), r.getY());
                            ++numChanges;
                        }
                    }

                    if (numChanges > 0)
                    {
                        updateBlockComponents (true);
                        updateConnectorComponents();
                    }

                    if (numEditorChanges > 0)
                        setSize (width, height);
                    return;
                }
                break;

                // Zoom options (commented don't look right yet)
                // case 50: setZoomScale (0.25); return; break;
                // case 51: setZoomScale (0.50); return; break;
                case 52:
                    setZoomScale (0.75);
                    return;
                    break;
                case 53:
                    setZoomScale (1.00);
                    return;
                    break;
                case 54:
                    setZoomScale (1.25);
                    return;
                    break;
                case 55:
                    setZoomScale (1.50);
                    return;
                    break;
                case 56:
                    setZoomScale (1.75);
                    return;
                    break;
                case 57:
                    setZoomScale (2.00);
                    return;
                    break;

                case 100: {
                    updateBlockComponents (true);
                    updateConnectorComponents();
                    return;
                }
                break;

                default:
                    failure = true;
                    break;
            }

            if (failure)
            {
                DBG ("[EL] unkown menu result: " << result);
            }
            else if (hasRequestedType)
            {
                const ValueTree requestedNode = graph.getNodesValueTree()
                                                    .getChildWithProperty (Tags::identifier, desc.fileOrIdentifier);
                const Node model (requestedNode, false);
                ViewHelpers::postMessageFor (this, new RemoveNodeMessage (model));
            }
            else
            {
                ViewHelpers::postMessageFor (this, new AddPluginMessage (graph, desc));
            }
        }
    }
    else
    {
        addAndMakeVisible (lasso);
        lasso.beginLasso (e, this);
    }
}

void GraphEditorComponent::mouseUp (const MouseEvent& e)
{
    lasso.endLasso();
    removeChildComponent (&lasso);
}

void GraphEditorComponent::mouseDrag (const MouseEvent& e)
{
    lasso.dragLasso (e);
}

void GraphEditorComponent::createNewPlugin (const PluginDescription* desc, int x, int y)
{
    DBG ("[EL] GraphEditorComponent::createNewPlugin(...)");
}

BlockComponent* GraphEditorComponent::getComponentForFilter (const uint32 filterID) const
{
    for (int i = getNumChildComponents(); --i >= 0;)
    {
        if (BlockComponent* const block = dynamic_cast<BlockComponent*> (getChildComponent (i)))
            if (block->filterID == filterID)
                return block;
    }

    return nullptr;
}

ConnectorComponent* GraphEditorComponent::getComponentForConnection (const Arc& arc) const
{
    for (int i = getNumChildComponents(); --i >= 0;)
    {
        if (ConnectorComponent* const c = dynamic_cast<ConnectorComponent*> (getChildComponent (i)))
            if (c->sourceFilterID == arc.sourceNode
                && c->destFilterID == arc.destNode
                && c->sourceFilterChannel == arc.sourcePort
                && c->destFilterChannel == arc.destPort)
                return c;
    }

    return nullptr;
}

PortComponent* GraphEditorComponent::findPinAt (const int x, const int y) const
{
    for (int i = getNumChildComponents(); --i >= 0;)
    {
        if (BlockComponent* block = dynamic_cast<BlockComponent*> (getChildComponent (i)))
        {
            if (PortComponent* pin = dynamic_cast<PortComponent*> (block->getComponentAt (x - block->getX(),
                                                                                          y - block->getY())))
                return pin;
        }
    }

    return nullptr;
}

void GraphEditorComponent::resized()
{
    updateBlockComponents (! areResizePositionsFrozen());
    updateConnectorComponents();
}

void GraphEditorComponent::changeListenerCallback (ChangeBroadcaster*)
{
    updateComponents();
}

void GraphEditorComponent::updateConnectorComponents()
{
    const ValueTree arcs = graph.getArcsValueTree();
    for (int i = getNumChildComponents(); --i >= 0;)
    {
        ConnectorComponent* const cc = dynamic_cast<ConnectorComponent*> (getChildComponent (i));
        if (cc != nullptr && cc != draggingConnector.get())
        {
            if (! Node::connectionExists (arcs, cc->sourceFilterID, (uint32) cc->sourceFilterChannel, cc->destFilterID, (uint32) cc->destFilterChannel, true))
            {
                delete cc;
            }
            else
            {
                // update cable or remove if can't get coordinates
                float x1, y1, x2, y2;
                if (cc->getPoints (x1, y1, x2, y2))
                    cc->update();
                else
                    delete cc;
            }
        }
    }
}

void GraphEditorComponent::updateBlockComponents (const bool doPosition)
{
    for (int i = getNumChildComponents(); --i >= 0;)
        if (auto* const block = dynamic_cast<BlockComponent*> (getChildComponent (i)))
        {
            block->update (doPosition);
        }
}

void GraphEditorComponent::stabilizeNodes()
{
    for (int i = getNumChildComponents(); --i >= 0;)
        if (auto* const block = dynamic_cast<BlockComponent*> (getChildComponent (i)))
        {
            block->update (false);
            block->repaint();
        }
}

void GraphEditorComponent::updateComponents (const bool doNodePositions)
{
    for (int i = graph.getNumConnections(); --i >= 0;)
    {
        const ValueTree c = graph.getConnectionValueTree (i);
        const Arc arc (Node::arcFromValueTree (c));
        ConnectorComponent* connector = getComponentForConnection (arc);

        if (connector == nullptr)
        {
            connector = new ConnectorComponent (graph);
            addAndMakeVisible (connector, i);
        }

        connector->setGraph (this->graph);
        connector->setInput (arc.sourceNode, arc.sourcePort);
        connector->setOutput (arc.destNode, arc.destPort);
    }

    for (int i = graph.getNumNodes(); --i >= 0;)
    {
        const Node node (graph.getNode (i));
        BlockComponent* comp = getComponentForFilter (node.getNodeId());
        if (comp == nullptr)
        {
            comp = createBlock (node);
            jassert (comp != nullptr);
            addAndMakeVisible (comp, i + 10000);
        }
    }

    updateBlockComponents (doNodePositions);
    updateConnectorComponents();
}

Rectangle<int> GraphEditorComponent::getRequiredSpace() const
{
    Rectangle<int> r;
    r.setX (0);
    r.setY (0);
    for (int i = getNumChildComponents(); --i >= 0;)
    {
        if (auto* const block = dynamic_cast<BlockComponent*> (getChildComponent (i)))
        {
            if (block->getRight() > r.getWidth())
                r.setWidth (block->getRight());
            if (block->getBottom() > r.getHeight())
                r.setHeight (block->getBottom());
        }
    }
    return r;
}

void GraphEditorComponent::beginConnectorDrag (const uint32 sourceNode, const int sourceFilterChannel, const uint32 destNode, const int destFilterChannel, const MouseEvent& e)
{
    draggingConnector.reset (dynamic_cast<ConnectorComponent*> (e.originalComponent));
    if (draggingConnector == nullptr)
        draggingConnector.reset (new ConnectorComponent (graph));

    draggingConnector->setGraph (this->graph);
    draggingConnector->setInput (sourceNode, sourceFilterChannel);
    draggingConnector->setOutput (destNode, destFilterChannel);
    draggingConnector->setAlwaysOnTop (true);
    addAndMakeVisible (draggingConnector.get());
    draggingConnector->toFront (false);

    dragConnector (e);
}

void GraphEditorComponent::dragConnector (const MouseEvent& e)
{
    const MouseEvent e2 (e.getEventRelativeTo (this));

    if (draggingConnector != nullptr)
    {
        draggingConnector->setTooltip (String());

        int x = e2.x;
        int y = e2.y;

        if (PortComponent* const pin = findPinAt (x, y))
        {
            uint32 srcFilter = draggingConnector->sourceFilterID;
            int srcChannel = draggingConnector->sourceFilterChannel;
            uint32 dstFilter = draggingConnector->destFilterID;
            int dstChannel = draggingConnector->destFilterChannel;

            if (srcFilter == 0 && ! pin->isInput())
            {
                srcFilter = pin->getNodeId();
                srcChannel = pin->getPortIndex();
            }
            else if (dstFilter == 0 && pin->isInput())
            {
                dstFilter = pin->getNodeId();
                dstChannel = pin->getPortIndex();
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

Component* GraphEditorComponent::createContainerForNode (NodeObjectPtr node, bool useGenericEditor)
{
    if (AudioProcessorEditor* ed = createEditorForNode (node, useGenericEditor))
        if (Component* comp = wrapAudioProcessorEditor (ed, node))
            return comp;
    return nullptr;
}

Component* GraphEditorComponent::wrapAudioProcessorEditor (AudioProcessorEditor* ed, NodeObjectPtr) { return ed; }

AudioProcessorEditor* GraphEditorComponent::createEditorForNode (NodeObjectPtr node, bool useGenericEditor)
{
    std::unique_ptr<AudioProcessorEditor> ui = nullptr;

    if (! useGenericEditor)
    {
        if (auto* proc = node->getAudioProcessor())
            ui.reset (proc->createEditorIfNeeded());
        if (ui == nullptr)
            useGenericEditor = true;
    }

    if (useGenericEditor)
        ui.reset (new GenericAudioProcessorEditor (node->getAudioProcessor()));

    return (nullptr != ui) ? ui.release() : nullptr;
}

void GraphEditorComponent::endDraggingConnector (const MouseEvent& e)
{
    if (draggingConnector == nullptr)
        return;

    draggingConnector->setTooltip (String());

    const MouseEvent e2 (e.getEventRelativeTo (this));

    uint32 srcFilter = draggingConnector->sourceFilterID;
    int srcChannel = draggingConnector->sourceFilterChannel;
    uint32 dstFilter = draggingConnector->destFilterID;
    int dstChannel = draggingConnector->destFilterChannel;

    draggingConnector = nullptr;

    if (PortComponent* const pin = findPinAt (e2.x, e2.y))
    {
        if (srcFilter == 0)
        {
            if (pin->isInput())
                return;

            srcFilter = pin->getNodeId();
            srcChannel = pin->getPortIndex();
        }
        else
        {
            if (! pin->isInput())
                return;

            dstFilter = pin->getNodeId();
            dstChannel = pin->getPortIndex();
        }

        connectPorts (graph, srcFilter, (uint32) srcChannel, dstFilter, (uint32) dstChannel);
    }
}

bool GraphEditorComponent::isInterestedInDragSource (const SourceDetails& details)
{
    if (details.description.toString() == "ccNavConcertinaPanel")
        return true;

    if (! details.description.isArray())
        return false;

    if (auto* a = details.description.getArray())
    {
        const var type (a->getFirst());
        return type == var ("plugin");
    }

    return false;
}

void GraphEditorComponent::itemDropped (const SourceDetails& details)
{
    lastDropX = (float) details.localPosition.x / (float) getWidth();
    lastDropY = (float) details.localPosition.y / (float) getHeight();

    if (const auto* a = details.description.getArray())
    {
        auto& plugs (ViewHelpers::getGlobals (this)->getPluginManager());

        if (const auto t = plugs.getKnownPlugins().getTypeForIdentifierString (a->getUnchecked (1).toString()))
        {
            ScopedPointer<AddPluginMessage> message = new AddPluginMessage (graph, *t);
            auto& builder (message->builder);

            if (ModifierKeys::getCurrentModifiersRealtime().isAltDown())
            {
                const auto audioInputNode = graph.getIONode (PortType::Audio, true);
                const auto midiInputNode = graph.getIONode (PortType::Midi, true);
                builder.addChannel (audioInputNode, PortType::Audio, 0, 0, false);
                builder.addChannel (audioInputNode, PortType::Audio, 1, 1, false);
                builder.addChannel (midiInputNode, PortType::Midi, 0, 0, false);
            }

            if (ModifierKeys::getCurrentModifiersRealtime().isCommandDown())
            {
                const auto audioOutputNode = graph.getIONode (PortType::Audio, false);
                const auto midiOutNode = graph.getIONode (PortType::Midi, false);
                builder.addChannel (audioOutputNode, PortType::Audio, 0, 0, true);
                builder.addChannel (audioOutputNode, PortType::Audio, 1, 1, true);
                builder.addChannel (midiOutNode, PortType::Midi, 0, 0, true);
            }

            postMessage (message.release());
        }
    }
    else if (details.description.toString() == "ccNavConcertinaPanel")
    {
        auto* nav = ViewHelpers::getNavigationConcertinaPanel (this);
        if (auto* panel = (nav) ? nav->findPanel<DataPathTreeComponent>() : nullptr)
        {
            File file = panel->getSelectedFile();
            if (file.hasFileExtension ("els"))
            {
#ifndef EL_SOLO
                postMessage (new OpenSessionMessage (file));
#endif
            }
            else if (file.hasFileExtension ("elg") || file.hasFileExtension ("elpreset"))
            {
                const Node node (Node::parse (file));
                bool wasHandled = false;

#if defined(EL_SOLO)
                // SE and LT Should just open graphs instead of trying to embed them
                if (file.hasFileExtension (".elg"))
                {
                    if (auto* cc = ViewHelpers::findContentComponent (this))
                        if (auto* gc = cc->getAppController().findChild<GraphController>())
                            gc->openGraph (file);
                    wasHandled = true;
                }
#endif

                if (! wasHandled && node.isValid())
                {
                    std::unique_ptr<AddNodeMessage> message (new AddNodeMessage (node, graph, file));

                    auto& builder (message->builder);
                    if (ModifierKeys::getCurrentModifiersRealtime().isAltDown())
                    {
                        const auto audioInputNode = graph.getIONode (PortType::Audio, true);
                        const auto midiInputNode = graph.getIONode (PortType::Midi, true);
                        builder.addChannel (audioInputNode, PortType::Audio, 0, 0, false);
                        builder.addChannel (audioInputNode, PortType::Audio, 1, 1, false);
                        builder.addChannel (midiInputNode, PortType::Midi, 0, 0, false);
                    }

                    if (ModifierKeys::getCurrentModifiersRealtime().isCommandDown())
                    {
                        const auto audioOutputNode = graph.getIONode (PortType::Audio, false);
                        const auto midiOutNode = graph.getIONode (PortType::Midi, false);
                        builder.addChannel (audioOutputNode, PortType::Audio, 0, 0, true);
                        builder.addChannel (audioOutputNode, PortType::Audio, 1, 1, true);
                        builder.addChannel (midiOutNode, PortType::Midi, 0, 0, true);
                    }
                    postMessage (message.release());
                }
            }
        }
    }
}

void GraphEditorComponent::valueTreeChildAdded (ValueTree& parent, ValueTree& child)
{
    if (child.hasType (Tags::node))
    {
        child.setProperty ("relativeX", verticalLayout ? lastDropX : lastDropY, 0);
        child.setProperty ("relativeY", verticalLayout ? lastDropY : lastDropX, 0);
        auto* comp = createBlock (Node (child, false));
        addAndMakeVisible (comp, 20000);
        comp->update();
    }
    else if (child.hasType (Tags::arc) || child.hasType (Tags::nodes) || child.hasType (Tags::arcs))
    {
        updateComponents();
    }
    else if (child.hasType (Tags::ports))
    {
        const Node node (parent, false);
        for (int i = 0; i < getNumChildComponents(); ++i)
            if (auto* const filter = dynamic_cast<BlockComponent*> (getChildComponent (i)))
                filter->update();
        updateConnectorComponents();
    }
}

void GraphEditorComponent::findLassoItemsInArea (Array<uint32>& itemsFound, 
                                                 const Rectangle<int>& area)
{
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        if (auto* block = dynamic_cast<BlockComponent*> (getChildComponent (i)))
            if (area.intersects (block->getBounds()))
                { itemsFound.add (block->node.getNodeId()); block->repaint(); }
    }
}

void GraphEditorComponent::selectNode (const Node& nodeToSelect)
{
    if (ignoreNodeSelected)
        return;

    for (int i = 0; i < graph.getNumNodes(); ++i)
    {
        auto node = graph.getNode (i);
        if (node == nodeToSelect)
        {
            selectedNodes.selectOnly (nodeToSelect.getNodeId());
            updateSelection();
            if (auto* cc = ViewHelpers::findContentComponent (this))
            {
                auto* gui = cc->getAppController().findChild<GuiController>();
                if (gui->getSelectedNode() != nodeToSelect)
                    gui->selectNode (nodeToSelect);
            }

            break;
        }
    }
}

void GraphEditorComponent::deleteSelectedNodes()
{
    NodeArray toRemove;
    for (const auto& nodeId : selectedNodes)
    {
        const auto node = graph.getNodeById (nodeId);
        toRemove.add (node);
    }

    ViewHelpers::postMessageFor (this, new RemoveNodeMessage (toRemove));
    selectedNodes.deselectAll();
}

void GraphEditorComponent::setSelectedNodesCompact (bool selected)
{
#if 0
    int nchanged = 0;
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        auto* block = dynamic_cast<BlockComponent*> (getChildComponent (i));
        if (nullptr == block)
            continue;
        if (! selectedNodes.getItemArray().contains (block->node.getNodeId()))
            continue;
        block->compact.removeListener (block);
        block->compact.setValue (selected);
        block->compact.addListener (block);
        block->update (false, false);
        ++nchanged;
    }

    if (nchanged > 0)
        updateConnectorComponents();
#endif
}

void GraphEditorComponent::setZoomScale (float scale)
{
    if (scale == zoomScale)
        return;

    zoomScale = scale;
    updateComponents();
    if (onZoomChanged)
        onZoomChanged();
}

void GraphEditorComponent::updateSelection()
{
    for (int i = getNumChildComponents(); --i >= 0;)
        if (auto* const block = dynamic_cast<BlockComponent*> (getChildComponent (i)))
            block->repaint();
}

BlockComponent* GraphEditorComponent::createBlock (const Node& node)
{
    if (auto* cc = ViewHelpers::findContentComponent (this))
        return factory->createBlockComponent (cc->getAppController(), node);
    jassertfalse;
    return nullptr;
}

} // namespace Element

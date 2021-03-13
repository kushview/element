/*
    This file is part of Element
    Copyright (C) 2019-2020  Kushview, LLC.  All rights reserved.

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

#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/views/NodePortsTableView.h"
#include "gui/BlockComponent.h"
#include "gui/Buttons.h"
#include "gui/ContentComponent.h"
#include "gui/ContextMenus.h"
#include "gui/GraphEditorComponent.h"
#include "gui/NodeIOConfiguration.h"
#include "gui/ViewHelpers.h"
#include "session/Node.h"
#include "Globals.h"
#include "ScopedFlag.h"

namespace Element {

//=============================================================================
PortComponent::PortComponent (const Node& g, const Node& n,
                              const uint32 nid, const uint32 i,
                              const bool dir, const PortType t,
                              const bool v)
    : graph (g), node (n), nodeID (nid), port (i), 
      type (t), input (dir), vertical (v)
{
    if (const GraphNodePtr obj = node.getGraphNode())
    {
        const Port p (node.getPort ((int) port));
        String tip = p.getName();
        
        if (tip.isEmpty())
        {
            if (node.isAudioInputNode())
            {
                tip = "Input " + String (port + 1);
            }
            else if (node.isAudioOutputNode())
            {
                tip = "Output " + String (port + 1);
            }
            else
            {
                tip = (input ? "Input " : "Output ") + String (port + 1);
            }
        }

        setTooltip (tip);
    }

    setSize (16, 16);
}

PortComponent::~PortComponent() {}

bool PortComponent::isInput() const noexcept         { return input; }
uint32 PortComponent::getNodeId() const noexcept     { return nodeID; }
uint32 PortComponent::getPortIndex() const noexcept  { return port; }

Colour PortComponent::getColor() const noexcept
{
    switch (this->type)
    {
        case PortType::Audio:   return Colours::lightgreen; break;
        case PortType::Control: return Colours::lightblue;  break;
        case PortType::Midi:    return Colours::orange;     break;
        default:
            break;
    }
    
    return Colours::red;
}

void PortComponent::paint (Graphics& g)
{
    g.setColour (getColor());
    g.fillEllipse (getLocalBounds().toFloat());
    g.setColour (Colours::black);
    g.drawEllipse (getLocalBounds().toFloat(), 0.5f);
}

void PortComponent::mouseDown (const MouseEvent& e)
{
    if (! isEnabled())
        return;
    getGraphEditor()->beginConnectorDrag (
        input ? 0 : nodeID, port,
        input ? nodeID : 0, port,
        e);
}

void PortComponent::mouseDrag (const MouseEvent& e)
{
    if (! isEnabled())
        return;
    getGraphEditor()->dragConnector (e);
}

void PortComponent::mouseUp (const MouseEvent& e)
{
    if (! isEnabled())
        return;
    getGraphEditor()->endDraggingConnector (e);
}
    
GraphEditorComponent* PortComponent::getGraphEditor() const noexcept
{
    return findParentComponentOfClass<GraphEditorComponent>();
}

//=============================================================================

BlockComponent::BlockComponent (const Node& graph_, const Node& node_, const bool vertical_)
    : filterID (node_.getNodeId()), graph (graph_), node (node_), font (11.0f)
{
    setBufferedToImage (true);
    nodeEnabled = node.getPropertyAsValue (Tags::enabled);
    nodeEnabled.addListener (this);
    nodeName = node.getPropertyAsValue (Tags::name);
    nodeName.addListener (this);

    shadow.setShadowProperties (DropShadow (Colours::black.withAlpha (0.5f), 3, Point<int> (0, 1)));
    setComponentEffect (&shadow);

    addAndMakeVisible (configButton);
    configButton.setPath (getIcons().fasCog);
    configButton.addListener (this);

    addAndMakeVisible (powerButton);
    powerButton.setColour (SettingButton::backgroundOnColourId,
                            findColour (SettingButton::backgroundColourId));
    powerButton.setColour (SettingButton::backgroundColourId, Colors::toggleBlue);
    powerButton.getToggleStateValue().referTo (node.getPropertyAsValue (Tags::bypass));
    powerButton.setClickingTogglesState (true);
    powerButton.addListener (this);

    addAndMakeVisible (muteButton);
    muteButton.setYesNoText ("M", "M");
    muteButton.setColour (SettingButton::backgroundOnColourId, Colors::toggleRed);
    muteButton.getToggleStateValue().referTo (node.getPropertyAsValue (Tags::mute));
    muteButton.setClickingTogglesState (true);
    muteButton.addListener (this);

    hiddenPorts = node.getUIValueTree()
        .getOrCreateChildWithName("block", nullptr)
        .getPropertyAsValue("hiddenPorts", nullptr);
    hiddenPorts.addListener (this);

    setSize (170, 60);
}

BlockComponent::~BlockComponent() noexcept
{
    nodeEnabled.removeListener (this);
    nodeName.removeListener (this);
    hiddenPorts.removeListener (this);
    deleteAllPins();
}

void BlockComponent::moveBlockTo (double x, double y)
{
    node.setPosition (x, y);
    setPositionFromNode();
}

void BlockComponent::setPowerButtonVisible (bool visible)   { setButtonVisible (powerButton, visible); }
void BlockComponent::setConfigButtonVisible (bool visible)  { setButtonVisible (configButton, visible); }
void BlockComponent::setMuteButtonVisible (bool visible)    { setButtonVisible (muteButton, visible); }

void BlockComponent::valueChanged (Value& value)
{
    if (nodeEnabled.refersToSameSourceAs (value)) {
        repaint();
    } else if (nodeName.refersToSameSourceAs (value)) {
        setName (node.getName());
        update (false, false);
    } else if (hiddenPorts.refersToSameSourceAs (value)) {
        if (auto* ge = getGraphPanel())
            ge->updateComponents (false);
    }
}

void BlockComponent::handleAsyncUpdate()
{
    repaint();
}

void BlockComponent::buttonClicked (Button* b) 
{
    if (! isEnabled())
        return;

    GraphNodePtr obj = node.getGraphNode();
    auto* proc = (obj) ? obj->getAudioProcessor() : 0;
    if (! proc) return;

    if (b == &configButton && configButton.getToggleState())
    {
        configButton.setToggleState (false, dontSendNotification);
        // ioBox.clear();
    }
    else if (b == &configButton && !configButton.getToggleState())
    {
        auto* component = new NodeAudioBusesComponent (node, proc,
                ViewHelpers::findContentComponent (this));
        CallOutBox::launchAsynchronously (
            std::unique_ptr<Component> (component),
            configButton.getScreenBounds(), nullptr);
        // ioBox.setNonOwned (&box);
    }
    else if (b == &powerButton)
    {
        if (obj->isSuspended() != node.isBypassed())
            obj->suspendProcessing (node.isBypassed());
    }
    else if (b == &muteButton)
    {
        node.setMuted (muteButton.getToggleState());
    }
}

void BlockComponent::setPositionFromNode()
{
    if (! node.isValid())
        return;
    
    double x = 0.0, y = 0.0;
    auto* const panel = getGraphPanel();
    Component* parent = nullptr;
    if (panel != nullptr)
        parent = panel->findParentComponentOfClass<Viewport>();
    if (parent == nullptr)
        parent = panel;

    if (! node.hasPosition() && nullptr != parent)
    {
        node.getRelativePosition (x, y);
        x = x * (parent->getWidth())  - (getWidth() / 2);
        y = y * (parent->getHeight()) - (getHeight() / 2);
        node.setPosition (x, y);
    }
    else
    {
        node.getPosition (x, y);
    }

    setBounds ({ roundDoubleToInt (vertical ? x : y),
                 roundDoubleToInt (vertical ? y : x), 
                 getWidth(), getHeight() });
}

void BlockComponent::setNodePosition (const int x, const int y)
{
    if (vertical)
    {
        node.setRelativePosition ((x + getWidth() / 2) / (double) getParentWidth(),
                                  (y + getHeight() / 2) / (double) getParentHeight());
        node.setProperty (Tags::x, (double) x);
        node.setProperty (Tags::y, (double) y);
    }
    else
    {
        node.setRelativePosition ((y + getHeight() / 2) / (double) getParentHeight(),
                                  (x + getWidth() / 2) / (double) getParentWidth());
        node.setProperty (Tags::y, (double) x);
        node.setProperty (Tags::x, (double) y);
    }
}

void BlockComponent::deleteAllPins()
{
    for (int i = getNumChildComponents(); --i >= 0;)
        if (auto * c = dynamic_cast<PortComponent*> (getChildComponent(i)))
            delete c;
}

void BlockComponent::mouseDown (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    bool collapsedToggled = false;
    if (! vertical && getOpenCloseBox().contains (e.x, e.y))
    {
        node.setProperty (Tags::collapsed, !collapsed);
        update (false);
        getGraphPanel()->updateConnectorComponents();
        collapsedToggled = true;
        blockDrag = true;
    }

    originalPos = localPointToGlobal (Point<int>());
    toFront (true);
    dragging = false;
    auto* const panel = getGraphPanel();
    
    selectionMouseDownResult = panel->selectedNodes.addToSelectionOnMouseDown (node.getNodeId(), e.mods);
    if (auto* cc = ViewHelpers::findContentComponent (this))
    {
        ScopedFlag block (panel->ignoreNodeSelected, true);
        cc->getAppController().findChild<GuiController>()->selectNode (node);
    }

    if (! collapsedToggled)
    {
        if (e.mods.isPopupMenu())
        {
            auto* const world = ViewHelpers::getGlobals (this);
            auto& plugins (world->getPluginManager());
            NodePopupMenu menu (node);
            menu.addReplaceSubmenu (plugins);

            if (! node.isMidiIONode() && ! node.isMidiDevice())
            {
                menu.addSeparator();
                menu.addItem (10, "Ports...", true, false);
            }

            menu.addSeparator();
            menu.addOptionsSubmenu();
            
            if (world)
                menu.addPresetsMenu (world->getPresetCollection());
            
            const int result = menu.show();
            if (auto* message = menu.createMessageForResultCode (result))
            {
                ViewHelpers::postMessageFor (this, message);
                for (const auto& nodeId : getGraphPanel()->selectedNodes)
                {
                    if (nodeId == node.getNodeId ())
                        continue;
                    const Node selectedNode = graph.getNodeById (nodeId);
                    if (selectedNode.isValid())
                    {
                        if (nullptr != dynamic_cast<RemoveNodeMessage*> (message))
                        {
                            ViewHelpers::postMessageFor (this, new RemoveNodeMessage (selectedNode));
                        }
                    }
                }
            }
            else if (plugins.getKnownPlugins().getIndexChosenByMenu(result) >= 0)
            {
                const auto index = plugins.getKnownPlugins().getIndexChosenByMenu (result);
                if (const auto* desc = plugins.getKnownPlugins().getType (index))
                    ViewHelpers::postMessageFor (this, new ReplaceNodeMessage (node, *desc));
            }
            else
            {
                switch (result) 
                {
                    case 10: {
                        auto* component = new NodePortsTable();
                        component->setNode (node);
                        CallOutBox::launchAsynchronously (
                            std::unique_ptr<Component> (component),
                            getScreenBounds(),
                            nullptr);
                        break;
                    }
                }
            }
        }
    }

    repaint();
    getGraphPanel()->updateSelection();
}

void BlockComponent::mouseDrag (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    if (e.mods.isPopupMenu() || blockDrag)
        return;
    dragging = true;
    Point<int> pos (originalPos + Point<int> (e.getDistanceFromDragStartX(), e.getDistanceFromDragStartY()));
    
    if (getParentComponent() != nullptr)
        pos = getParentComponent()->getLocalPoint (nullptr, pos);
    
    setNodePosition (pos.getX(), pos.getY());
    setPositionFromNode();

    if (auto* const panel = getGraphPanel())
    {
        if (panel->onBlockMoved)
            panel->onBlockMoved (*this);
        panel->updateConnectorComponents();
    }
}

void BlockComponent::mouseUp (const MouseEvent& e)
{
    if (! isEnabled())
        return;
    auto* panel = getGraphPanel();
    
    if (panel)
        panel->selectedNodes.addToSelectionOnMouseUp (node.getNodeId(), e.mods,
                                                        dragging, selectionMouseDownResult);

    if (e.mouseWasClicked() && e.getNumberOfClicks() == 2)
        makeEditorActive();

    dragging = selectionMouseDownResult = blockDrag = false;   
}

void BlockComponent::updatePosition()
{
    node.getRelativePosition (relativeX, relativeY);
    vertical ? setCentreRelative (relativeX, relativeY)
             : setCentreRelative (relativeY, relativeX);
    getGraphPanel()->updateConnectorComponents();
}

void BlockComponent::makeEditorActive()
{
    if (node.isGraph())
    {
        // TODO: this can cause a crash, do it async
        if (auto* cc = ViewHelpers::findContentComponent (this))
            cc->setCurrentNode (node);
    }
    else if (node.hasProperty (Tags::missing))
    {
        String message = "This node is unavailable and running as a Placeholder.\n";
        message << node.getName() << " (" << node.getFormat().toString() 
                << ") could not be found for loading.";
        AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon, 
            node.getName(), message, "Ok");
    }
    else if (node.isValid())
    {
        ViewHelpers::presentPluginWindow (this, node);
    }
}

bool BlockComponent::hitTest (int x, int y)
{
    for (int i = getNumChildComponents(); --i >= 0;)
        if (getChildComponent(i)->getBounds().contains (x, y))
            return true;

    return vertical ? x >= 3 && x < getWidth() - 6 && y >= pinSize && y < getHeight() - pinSize
                    : y >= 3 && y < getHeight() - 6 && x >= pinSize && x < getWidth() - pinSize;
}

Rectangle<int> BlockComponent::getOpenCloseBox() const
{
    const auto box (getBoxRectangle());
    return { box.getX() + 5, box.getY() + 4, 16, 16 };
}

Rectangle<int> BlockComponent::getBoxRectangle() const
{
    #if 0
    // for pins with stems
    if (vertical)
    {
        const int x = 4;
        const int y = pinSize;
        const int w = getWidth() - x * 2;
        const int h = getHeight() - pinSize * 2;
        
        return Rectangle<int> (x, y, w, h);
    }

    const int x = pinSize;
    const int y = 4;
    const int w = getWidth() - pinSize * 2;
    const int h = getHeight() - y * 2;
    
    return { x, y, w, h };
    #else
    if (vertical)
    {
        return Rectangle<int> (
            0,
            pinSize / 2,
            getWidth(),
            getHeight() - pinSize
        ).reduced (2, 0);
    }

    return Rectangle<int> (
        pinSize / 2,
        0,
        getWidth() - pinSize,
        getHeight()
    ).reduced (0, 2);
    #endif
}

void BlockComponent::paintOverChildren (Graphics& g)
{
    ignoreUnused (g);
}

void BlockComponent::paint (Graphics& g)
{
    const float cornerSize = 2.4f;
    const auto box (getBoxRectangle());

    g.setColour (isEnabled() && node.isEnabled()
        ? LookAndFeel::widgetBackgroundColor.brighter (0.8) 
        : LookAndFeel::widgetBackgroundColor.brighter (0.2));
    g.fillRoundedRectangle (box.toFloat(), cornerSize);

    if (! vertical)
    {
        getLookAndFeel().drawTreeviewPlusMinusBox (
            g, getOpenCloseBox().toFloat(),
            LookAndFeel::widgetBackgroundColor.brighter (0.7), 
            ! collapsed, false); 
    }

    if (node.isMissing())
    {
        g.setColour (Colour (0xff333333));
        g.setFont (9.f);
        auto pr = box; pr.removeFromTop (6);
        g.drawFittedText ("(placeholder)", pr, Justification::centred, 2);
    }

    g.setColour (Colours::black);
    g.setFont (font);
    
    auto displayName = node.getDisplayName();
    auto subName = node.hasModifiedName() ? node.getPluginName() : String();
    
    if (node.getParentGraph().isRootGraph())
    {
        if (node.isAudioIONode())
        {
            // FIXME: uniform way to refresh node names
            //        see https://github.com/kushview/Element/issues/109
            subName = String();
        }
        else if (node.isMidiInputNode())
        {
            auto& midi = ViewHelpers::getGlobals(this)->getMidiEngine();
            if (midi.getNumActiveMidiInputs() <= 0)
                subName = "(no device)";
        }
    }

    if (vertical)
    {
        g.drawFittedText (displayName, box.getX() + 9, box.getY() + 2, box.getWidth(),
                                        18, Justification::centredLeft, 2);

        if (subName.isNotEmpty())
        {
            g.setFont (Font (8.f));
            g.drawFittedText (subName, box.getX() + 9, box.getY() + 10, box.getWidth(),
                                18, Justification::centredLeft, 2);
        }
    }
    else
    {
        g.drawFittedText (displayName, box.getX() + 20, box.getY() + 2, box.getWidth(),
                                        18, Justification::centredLeft, 2);
        if (subName.isNotEmpty())
        {
            g.setFont (Font (8.f));
            g.drawFittedText (subName, box.getX() + 20, box.getY() + 10, box.getWidth(),
                              18, Justification::centredLeft, 2);
        }
    }
    
    bool selected = getGraphPanel()->selectedNodes.isSelected (node.getNodeId());
    g.setColour (selected ? Colors::toggleBlue : Colours::grey);
    g.drawRoundedRectangle (box.toFloat(), cornerSize, 1.4);
}

void BlockComponent::resized()
{
    const auto box (getBoxRectangle());
    auto r = box.reduced(4, 2).removeFromBottom (14);
    
    {
        Component* buttons[] = { &configButton, &muteButton, &powerButton };
        for (int i = 0; i < 3; ++i)
            if (buttons[i]->isVisible())
                buttons[i]->setBounds (r.removeFromRight (16));
    }
    
    const int halfPinSize = pinSize / 2;
    if (vertical)
    {
        Rectangle<int> pri (box.getX() + 9, 0, getWidth(), pinSize);
        Rectangle<int> pro (box.getX() + 9, getHeight() - pinSize, getWidth(), pinSize);
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (PortComponent* const pc = dynamic_cast <PortComponent*> (getChildComponent(i)))
            {
                pc->setBounds (pc->isInput() ? pri.removeFromLeft (pinSize) 
                                                : pro.removeFromLeft (pinSize));
                pc->isInput() ? pri.removeFromLeft (pinSize * 1.25)
                                : pro.removeFromLeft (pinSize * 1.25);                  
            }
        }
    }
    else
    {
        Rectangle<int> pri (box.getX() - halfPinSize, 
                            box.getY() + 9, 
                            pinSize, 
                            box.getHeight());
        Rectangle<int> pro (box.getWidth(),
                            box.getY() + 9, 
                            pinSize, 
                            box.getHeight());
        float scale = collapsed ? 0.25f : 1.125f;
        int spacing = jmax (2, int (pinSize * scale));
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (PortComponent* const pc = dynamic_cast <PortComponent*> (getChildComponent(i)))
            {
                pc->setBounds (pc->isInput() ? pri.removeFromTop (pinSize) 
                                                : pro.removeFromTop (pinSize));
                pc->isInput() ? pri.removeFromTop (spacing)
                                : pro.removeFromTop (spacing);
            }
        }
    }
}

bool BlockComponent::getPortPos (const int index, const bool isInput, float& x, float& y)
{
    bool res = false;
    
    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        if (auto* const pc = dynamic_cast <PortComponent*> (getChildComponent (i)))
        {
            if (pc->getPortIndex() == index && isInput == pc->isInput())
            {
                x = getX() + pc->getX() + pc->getWidth() * 0.5f;
                y = getY() + pc->getY() + pc->getHeight() * 0.5f;
                res = true;
                break;
            }
        }
    }

    return res;
}

void BlockComponent::update (const bool doPosition, const bool forcePins)
{
    auto* const ged = getGraphPanel();
    if (nullptr == ged)
    {
        jassertfalse;
        return;
    }

    vertical = ged->isLayoutVertical();

    if (! node.getValueTree().getParent().hasType (Tags::nodes))
    {
        delete this;
        return;
    }

    collapsed = (bool) node.getProperty (Tags::collapsed, false);
    numIns = numOuts = 0;
    const auto numPorts = node.getNumPorts();
    for (int i = 0; i < numPorts; ++i)
    {
        const Port port (node.getPort (i));
        if (PortType::Control == port.getType() || port.isHiddenOnBlock())
            continue;
        
        if (port.isInput())
            ++numIns;
        else
            ++numOuts;
    }

    int w = roundToInt (120.0 * ged->getZoomScale());
    int h = roundToInt (46.0 * ged->getZoomScale());

    const int maxPorts = jmax (numIns, numOuts) + 1;
    
    if (vertical)
    {
        w = jmax (w, int(maxPorts * pinSize) + int(maxPorts * pinSize * 1.25f));
    }
    else
    {
        float scale = collapsed ? 0.25f : 1.125f;
        int endcap = collapsed ? 9 : -5;
        h = jmax (h, int(maxPorts * pinSize) + int(maxPorts * jmax(int(pinSize * scale), 2)) + endcap);
    }
    
    font.setHeight (11.f * ged->getZoomScale());
    int textWidth = font.getStringWidth (node.getDisplayName());
    textWidth += (vertical) ? 20 : 36;
    w = jmax (w, textWidth);
    setSize (w, h);
    setName (node.getDisplayName());

    if (doPosition)
    {
        setPositionFromNode();
    }
    else if (nullptr != getParentComponent())
    {
        // position is relative and parent might be resizing
        const auto b = getBoundsInParent();
        setNodePosition (b.getX(), b.getY());
    }

    if (forcePins || numIns != numInputs || numOuts != numOutputs)
    {
        numInputs  = numIns;
        numOutputs = numOuts;

        deleteAllPins();

        for (uint32 i = 0; i < (uint32) numPorts; ++i)
        {
            const Port port (node.getPort (i));
            const PortType t (port.getType());
            if (t == PortType::Control || port.isHiddenOnBlock())
                continue;

            const bool isInput (port.isInput());
            addAndMakeVisible (new PortComponent (graph, node, filterID, i, isInput, t, vertical));
        }

        resized();
    }

    repaint();
}

void BlockComponent::setButtonVisible (Button& b, bool v)
{
    if (b.isVisible() == v)
        return;
    b.setVisible (v);
    resized();
}

GraphEditorComponent* BlockComponent::getGraphPanel() const noexcept
{
    return findParentComponentOfClass<GraphEditorComponent>();
}


}

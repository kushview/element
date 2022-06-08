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
#include "gui/Artist.h"
#include "gui/BlockComponent.h"
#include "gui/Buttons.h"
#include "gui/ContentComponent.h"
#include "gui/ContextMenus.h"
#include "gui/GraphEditorComponent.h"
#include "gui/NodeIOConfiguration.h"
#include "gui/NodeEditorFactory.h"
#include "gui/ViewHelpers.h"
#include "session/Node.h"
#include "Globals.h"
#include "ScopedFlag.h"

namespace Element {

//=============================================================================
PortComponent::PortComponent (const Node& g, const Node& n, const uint32 nid, const uint32 i, const bool dir, const PortType t, const bool v)
    : graph (g), node (n), nodeID (nid), port (i), type (t), input (dir), vertical (v)
{
    if (const NodeObjectPtr obj = node.getObject())
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

bool PortComponent::isInput() const noexcept { return input; }
uint32 PortComponent::getNodeId() const noexcept { return nodeID; }
uint32 PortComponent::getPortIndex() const noexcept { return port; }

Colour PortComponent::getColor() const noexcept
{
    switch (this->type)
    {
        case PortType::Audio:
            return Colours::lightgreen;
            break;
        case PortType::Control:
            return Colours::lightblue;
            break;
        case PortType::Midi:
            return Colours::orange;
            break;
        default:
            break;
    }

    return Colours::red;
}

void PortComponent::paint (Graphics& g)
{   
    Path path;
    
    float start = 0.0, end = 0.0;
    if (vertical)
    {
        start = input ?  -90.f :  270.f;
        end   = input ? 90.f :   90.f;
    }
    else
    {
        start = input ?  180.f :   0.f;
        end   = input ?  360.f : 180.f;
    }

    path.addPieSegment(getLocalBounds().toFloat(), 
        degreesToRadians(start), degreesToRadians (end), 0);
    g.setColour (getColor());
    g.fillPath(path);
}

void PortComponent::mouseDown (const MouseEvent& e)
{
    if (! isEnabled())
        return;
    getGraphEditor()->beginConnectorDrag (
        input ? 0 : nodeID, port, input ? nodeID : 0, port, e);
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

    hiddenPorts = node.getBlockValueTree()
        .getPropertyAsValue (Tags::hiddenPorts, nullptr);
    hiddenPorts.addListener (this);

    displayModeValue = node.getBlockValueTree()
        .getPropertyAsValue (Tags::displayMode, nullptr);
    displayMode = getDisplayModeFromString (displayModeValue.toString());
    displayModeValue.addListener (this);

    customWidth  = node.getBlockValueTree().getProperty (Tags::width, customWidth);
    customHeight = node.getBlockValueTree().getProperty (Tags::height, customHeight);
    setSize (customWidth > 0 ? customWidth : 170, 
             customHeight > 0 ? customHeight : 60);
}

BlockComponent::~BlockComponent() noexcept
{
    nodeEnabled.removeListener (this);
    nodeName.removeListener (this);
    hiddenPorts.removeListener (this);
    displayModeValue.removeListener (this);
    deleteAllPins();
}

void BlockComponent::setDisplayMode (DisplayMode mode)
{
    if (mode == displayMode)
        return;
    displayMode = mode;
    updateSize();
}

void BlockComponent::moveBlockTo (double x, double y)
{
    setNodePosition (x, y);
    updatePosition();
}

bool BlockComponent::isSelected() const noexcept
{
    return selected;
}

void BlockComponent::setPowerButtonVisible (bool visible) { setButtonVisible (powerButton, visible); }
void BlockComponent::setConfigButtonVisible (bool visible) { setButtonVisible (configButton, visible); }
void BlockComponent::setMuteButtonVisible (bool visible) { setButtonVisible (muteButton, visible); }

void BlockComponent::valueChanged (Value& value)
{
    if (nodeEnabled.refersToSameSourceAs (value))
    {
        repaint();
    }
    else if (nodeName.refersToSameSourceAs (value))
    {
        setName (node.getName());
        update (false, false);
    }
    else if (hiddenPorts.refersToSameSourceAs (value))
    {
        if (auto* ge = getGraphPanel())
            ge->updateComponents (false);
    }
    else if (displayModeValue.refersToSameSourceAs (value))
    {
        update (false, false);
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

    NodeObjectPtr obj = node.getObject();
    auto* proc = (obj) ? obj->getAudioProcessor() : 0;
    if (! proc)
        return;

    if (b == &configButton && configButton.getToggleState())
    {
        configButton.setToggleState (false, dontSendNotification);
        // ioBox.clear();
    }
    else if (b == &configButton && ! configButton.getToggleState())
    {
        auto* component = new NodeAudioBusesComponent (node, proc, ViewHelpers::findContentComponent (this));
        CallOutBox::launchAsynchronously (
            std::unique_ptr<Component> (component),
            configButton.getScreenBounds(),
            nullptr);
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

void BlockComponent::deleteAllPins()
{
    for (int i = getNumChildComponents(); --i >= 0;)
        if (auto* c = dynamic_cast<PortComponent*> (getChildComponent (i)))
            delete c;
}

void BlockComponent::changeListenerCallback (ChangeBroadcaster* broadcaster)
{
    if (broadcaster == &colorSelector)
    {
        color = colorSelector.getCurrentColour().withAlpha (1.0f);
        node.getUIValueTree().setProperty ("color", color.toString(), nullptr);
        
        forEachSibling ([this] (BlockComponent& sibling) {
            if (!sibling.isSelected() || sibling.color == color)
                return;
            sibling.color = color;
            sibling.node.getUIValueTree().setProperty ("color",
                                                       sibling.color.toString(),
                                                       nullptr);
            sibling.repaint();
        });

        repaint();
    }
}

void BlockComponent::mouseDown (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    originalPos = localPointToGlobal (Point<int>());
    originalBounds = getBounds();
    toFront (true);
    dragging = false;
    auto* const panel = getGraphPanel();

    selectionMouseDownResult = panel->selectedNodes.addToSelectionOnMouseDown (node.getNodeId(), e.mods);
    if (auto* cc = ViewHelpers::findContentComponent (this))
    {
        ScopedFlag block (panel->ignoreNodeSelected, true);
        cc->getAppController().findChild<GuiController>()->selectNode (node);
    }

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
        menu.addColorSubmenu (colorSelector);
        menu.addDisplaySubmenu (menu);

        menu.addOptionsSubmenu();

        if (world)
            menu.addPresetsMenu (world->getPresetCollection());

        colorSelector.setCurrentColour (Colour::fromString (
            node.getUIValueTree().getProperty ("color", color.toString()).toString()));
        colorSelector.addChangeListener (this);
        const int result = menu.show();
        colorSelector.removeChangeListener (this);

        if (auto* message = menu.createMessageForResultCode (result))
        {
            ViewHelpers::postMessageFor (this, message);
            for (const auto& nodeId : getGraphPanel()->selectedNodes)
            {
                if (nodeId == node.getNodeId())
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
        else if (plugins.getKnownPlugins().getIndexChosenByMenu (result) >= 0)
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

    repaint();
    getGraphPanel()->updateSelection();
}

void BlockComponent::mouseMove (const MouseEvent& e)
{
    Component::mouseMove (e);
    if (getCornerResizeBox().toFloat().contains (e.position))
    {
        if (! mouseInCornerResize)
        {
            mouseInCornerResize = true;
            repaint();
        }
    }
    else
    {
        if (mouseInCornerResize)
        {
            mouseInCornerResize = false;
            repaint();
        }
    }
}

void BlockComponent::mouseDrag (const MouseEvent& e)
{
    if (! isEnabled())
        return;

    if (e.mods.isPopupMenu() || blockDrag)
        return;
    
    if (mouseInCornerResize)
    {
        setCustomSize (originalBounds.getWidth()  + e.getDistanceFromDragStartX(),
                       originalBounds.getHeight() + e.getDistanceFromDragStartY());
        return;
    }

    // if (std::abs (e.getDistanceFromDragStartX()) < 2 && abs (e.getDistanceFromDragStartY()) < 2)
    //     return;

    dragging = true;
    int deltaX = e.getDistanceFromDragStartX();
    int deltaY = e.getDistanceFromDragStartY();

    Point<int> pos (originalPos + Point<int> (deltaX, deltaY));
    if (getParentComponent() != nullptr)
        pos = getParentComponent()->getLocalPoint (nullptr, pos);

    moveBlockTo (pos.getX(), pos.getY());

    if (auto* const panel = getGraphPanel())
    {
        if (panel->onBlockMoved)
            panel->onBlockMoved (*this);
        
        int dx = deltaX - lastDragDeltaX;
        int dy = deltaY - lastDragDeltaY;

        for (int i = 0; i < panel->getNumChildComponents(); ++i)
        {
            auto* block = dynamic_cast<BlockComponent*> (panel->getChildComponent (i));
            if (block == nullptr || block == this || ! block->isSelected())
                continue;
            
            auto bp = block->getNodePosition();
            if (! vertical) 
                std::swap (bp.x, bp.y);
        
            block->moveBlockTo (roundToIntAccurate (bp.x + dx), 
                                roundToIntAccurate (bp.y + dy));
        }

        panel->updateConnectorComponents();
    }

    lastDragDeltaX = deltaX;
    lastDragDeltaY = deltaY;
}

void BlockComponent::mouseUp (const MouseEvent& e)
{
    dragging = selectionMouseDownResult = blockDrag = false;
    lastDragDeltaX = lastDragDeltaY = 0;
    if (! isEnabled())
        return;
    auto* panel = getGraphPanel();

    if (panel)
        panel->selectedNodes.addToSelectionOnMouseUp (node.getNodeId(), e.mods, dragging, selectionMouseDownResult);

    if (e.mouseWasClicked() && e.getNumberOfClicks() == 2)
        makeEditorActive();
}

void BlockComponent::setSelectedInternal (bool status)
{
    if (selected == status)
        return;
    selected = status;
    repaint();
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
                                          node.getName(),
                                          message,
                                          "Ok");
    }
    else if (node.isValid())
    {
        ViewHelpers::presentPluginWindow (this, node);
    }
}

bool BlockComponent::hitTest (int x, int y)
{
    for (int i = getNumChildComponents(); --i >= 0;)
        if (getChildComponent (i)->getBounds().contains (x, y))
            return true;
    return getBoxRectangle().contains (x, y);
}

Rectangle<int> BlockComponent::getBoxRectangle() const
{
    return getLocalBounds().reduced (pinSize / 2);
}

Rectangle<int> BlockComponent::getCornerResizeBox() const
{
    auto r = getBoxRectangle();
    return { r.getRight() - 14, r.getBottom() - 14, 12, 12 };
}

void BlockComponent::paintOverChildren (Graphics& g)
{
    ignoreUnused (g);
}

void BlockComponent::paint (Graphics& g)
{
    const float cornerSize = 2.4f;
    const auto box (getBoxRectangle());
    const int colorBarHeight = vertical ? 20 : 18;
    bool colorize = color != Colour(0x00000000);
    Colour bgc = isEnabled() && node.isEnabled()
            ? LookAndFeel::widgetBackgroundColor.brighter (0.8)
            : LookAndFeel::widgetBackgroundColor.brighter (0.2);
    
    auto barColor = isEnabled() && node.isEnabled() ? color : color.darker (.1);

    if (isSelected())
    {
        bgc = bgc.brighter (0.55);
    }

    if (colorize)
    {
        switch (displayMode)
        {
            case Compact:
            case Small: {
                g.setColour (selected ? barColor.brighter (0.275) : barColor);
                g.fillRoundedRectangle (box.toFloat(), cornerSize);
                break;
            }
            case Embed:
            case Normal: {
                auto b1 = box;
                auto b2 = b1.removeFromTop (colorBarHeight);
                g.setColour (barColor);
                Path path;
                path.addRoundedRectangle (b2.getX(), b2.getY(), b2.getWidth(), b2.getHeight(),
                    cornerSize, cornerSize, true, true, false, false);
                g.fillPath (path);

                path.clear();
                g.setColour (bgc);
                path.addRoundedRectangle (b1.getX(), b1.getY(), b1.getWidth(), b1.getHeight(),
                    cornerSize, cornerSize, false, false, true, true);
                g.fillPath (path);
                break;
            }
        }
    }
    else
    {
        g.setColour (bgc);
        g.fillRoundedRectangle (box.toFloat(), cornerSize);
    }

    if (node.isMissing())
    {
        g.setColour (Colour (0xff333333));
        g.setFont (9.f);
        auto pr = box;
        pr.removeFromTop (6);
        g.drawFittedText ("(placeholder)", pr, Justification::centred, 2);
    }

    if (colorize)
        g.setColour (Colours::white.overlaidWith(color).contrasting());
    else
        g.setColour (Colours::black);
    
    g.setFont (Font (12.f));

    auto displayName = node.getDisplayName();
    auto subName = node.hasModifiedName() ? node.getPluginName() : String();

    if (node.getParentGraph().isRootGraph())
    {
        if (node.isAudioIONode())
        {
            subName = String();
        }
        else if (node.isMidiInputNode())
        {
            auto mode  = ViewHelpers::getGuiController (this)->getRunMode();
            auto& midi = ViewHelpers::getGlobals (this)->getMidiEngine();
            if (mode != RunMode::Plugin && midi.getNumActiveMidiInputs() <= 0)
                subName = "(no device)";
        }
    }

    if (vertical)
    {
        switch (displayMode)
        {
            case Small:
            case Normal:
            case Embed: {
                int y = box.getY() + 2;
                g.drawFittedText (displayName, box.getX(), y, box.getWidth(), 18, Justification::centred, 2);

                if (subName.isNotEmpty())
                {
                    g.setColour (Colours::black);
                    g.setFont (Font (9.f));
                    y += colorBarHeight;
                    g.drawFittedText (subName, box.getX(), y, 
                        box.getWidth(), 9, Justification::centred, 2);
                }
                break;
            }
            case Compact: {
                g.drawFittedText (displayName, box.getX(), box.getY(), 
                box.getWidth(), box.getHeight(), Justification::centred, 2);
                break;
            }
        }
    }
    else
    {
        switch (displayMode) {
            case Normal:
            case Embed: {
                int y = box.getY();
                g.drawFittedText (displayName, box.getX(), y, box.getWidth(), 18, Justification::centred, 2);

                if (subName.isNotEmpty())
                {
                    g.setColour (Colours::black);
                    g.setFont (Font (9.f));
                    y += colorBarHeight;
                    g.drawFittedText (subName, box.getX(), y, box.getWidth(), 13, Justification::centred, 2);
                }
                break;
            }
            case Small: {
                g.drawFittedText (displayName, box, Justification::centred, 2);
                break;
            }
            case Compact: {
                Artist::drawVerticalText (g, displayName, getLocalBounds(), Justification::centred);
                break;
            }
        }
    }

    if (mouseInCornerResize)
    {
        auto cbox = getCornerResizeBox();
        g.setOrigin (cbox.getPosition());
        getLookAndFeel().drawCornerResizer (g, 12, 12, true, false);
    }
}

void BlockComponent::resized()
{
    const auto box (getBoxRectangle());
    auto r = box.reduced (4, 2).removeFromBottom (14);
    const int halfPinSize = pinSize / 2;
    
    {
        Component* buttons[] = { &configButton, &muteButton, &powerButton };
        for (int i = 0; i < 3; ++i)
            if (buttons[i]->isVisible())
                buttons[i]->setBounds (r.removeFromLeft (16));
    }

    if (displayMode == Embed && embedded) {
        auto er = box;
        er.removeFromTop (vertical ? 20 : 18);
        er.removeFromBottom (18);
        embedded->setBounds (er.reduced (1));
    }

    if (vertical)
    {
        Rectangle<int> pri (box.getX() + 9, 0, getWidth(), pinSize);
        Rectangle<int> pro (box.getX() + 9, getHeight() - pinSize, getWidth(), pinSize);

        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (PortComponent* const pc = dynamic_cast<PortComponent*> (getChildComponent (i)))
            {
                pc->setBounds (pc->isInput() ? pri.removeFromLeft (pinSize)
                                             : pro.removeFromLeft (pinSize));
                pc->isInput() ? pri.removeFromLeft (pinSpacing)
                              : pro.removeFromLeft (pinSpacing);
            }
        }
    }
    else
    {
        int startY = box.getY() + 22;
        if (displayMode == Compact || displayMode == Small)
        {
            startY = (jmax(numIns, numOuts) * (pinSpacing + pinSize)) - pinSpacing;
            startY = box.getY() + ((box.getHeight() - startY) / 2);
        }
        
        Rectangle<int> pri (box.getX() - halfPinSize,
                            startY,
                            pinSize,
                            box.getHeight());
        Rectangle<int> pro (pri.withX (box.getWidth() - 1));
        
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (PortComponent* const pc = dynamic_cast<PortComponent*> (getChildComponent (i)))
            {
                pc->setBounds (pc->isInput() ? pri.removeFromTop (pinSize)
                                             : pro.removeFromTop (pinSize));
                pc->isInput() ? pri.removeFromTop (pinSpacing)
                              : pro.removeFromTop (pinSpacing);
            }
        }
    }
}

bool BlockComponent::getPortPos (const int index, const bool isInput, float& x, float& y)
{
    bool res = false;

    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        if (auto* const pc = dynamic_cast<PortComponent*> (getChildComponent (i)))
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

    if (! node.getValueTree().getParent().hasType (Tags::nodes))
    {
        delete this;
        return;
    }

    vertical = ged->isLayoutVertical();

    auto displayModeChanged = displayMode != getDisplayModeFromString (displayModeValue.getValue());
    displayMode = getDisplayModeFromString (displayModeValue.getValue());
    if (displayMode == Compact || displayMode == Small)
    {
        setMuteButtonVisible (false);
        setConfigButtonVisible (false);
        setPowerButtonVisible (false);
    }
    else 
    {
        setMuteButtonVisible (true);
        setConfigButtonVisible (true);
        setPowerButtonVisible (true);
    }
    
    if (displayMode == Embed)
    {
        if (embedded == nullptr)
        {
            if (auto* ui = ViewHelpers::getGuiController(this))
            {
                NodeEditorFactory factory (*ui);
                if (auto e = factory.instantiate (node, NodeEditorPlacement::NavigationPanel))
                    embedded = e.release();
                // else
                //     embedded = NodeEditorFactory::createAudioProcessorEditor (node).release();
            }
            if (embedded != nullptr)
                addAndMakeVisible (embedded.get());
        }
    }
    else
    {
        if (embedded != nullptr)
            embedded = nullptr;
    }

    updatePins (forcePins);
    updateSize();
    setName (node.getDisplayName());

    if (doPosition)
    {
        updatePosition();
    }
    else if (nullptr != getParentComponent())
    {
        // position is relative and parent might be resizing
        const auto b = getBoundsInParent();
        setNodePosition (b.getX(), b.getY());
    }

    if (node.getUIValueTree().hasProperty ("color"))
    {
        color = Colour::fromString (node.getUIValueTree().getProperty ("color").toString());
    }
    else
    {
        color = Colour (0x00000000);
    }

    repaint();
    resized();
}

void BlockComponent::getMinimumSize (int& width, int& height)
{
    auto *ged = getGraphPanel();
    if (! ged)
        return;
    
    int w = roundToInt ((!vertical ? 120.0 : 90) * ged->getZoomScale());
    int h = roundToInt (46.0 * ged->getZoomScale());
    const int maxPorts = jmax (numIns, numOuts) + 1;
    font.setHeight (11.f * ged->getZoomScale());
    int textWidth = font.getStringWidth (node.getDisplayName());
    textWidth += (vertical) ? 20 : 36;
    pinSpacing = int (pinSize * (displayMode == Compact || displayMode == Small ? 0.5f : 0.9f));
    int pinSpaceNeeded = int (maxPorts * pinSize) + int (maxPorts * pinSpacing);

    if (vertical)
    {
        w = jmax (w, int (maxPorts * pinSize) + int (maxPorts * pinSpacing));
        h = 60;

        if (displayMode == Compact)
        {
            h = (pinSize * 2) + 20;
        }

        w = jmax (w, textWidth);
    }
    else
    {
        if (displayMode == Compact)
        {
            w = (pinSize * 2) + 24;
            auto pinSpace = pinSpaceNeeded + pinSize;
            if (pinSpace >= textWidth)
                h = pinSpace;
            else
                h = textWidth;
        }
        else if (displayMode == Small)
        {
            w = textWidth + 6;
            h = pinSpaceNeeded + pinSize;
        }
        else
        {
            int endcap = displayMode == Compact ? 9 : 12;
            w = jmax (w, textWidth);
            h = jmax (h, (maxPorts * pinSize) + (maxPorts * jmax (pinSpacing, 2) + endcap));
        }
    }

    width = w;
    height = h;
}

void BlockComponent::updateSize()
{
    auto *ged = getGraphPanel();
    if (! ged)
        return;
    
    customWidth = (int) node.getBlockValueTree()
        .getProperty (Tags::width, customWidth);
    customHeight = (int) node.getBlockValueTree()
        .getProperty (Tags::height, customHeight);
    
    int minW = 0, minH = 0;
    getMinimumSize (minW, minH);
    jassert (minW > 0 && minH > 0);
    switch (displayMode)
    {
        case Compact:
        case Small: {
            setSize (minW, minH);
            break;
        }
        case Embed: {
            if (embedded != nullptr) {
                setSize (embedded->getWidth() + pinSize,
                         embedded->getHeight() + 18 + 18);
            }
            break;
        }
        case Normal: {
            setSize (customWidth > 0 ? customWidth : minW,
                     customHeight > 0 ? customHeight : minH);
            break;
        }
    }
}

void BlockComponent::setCustomSize (int width, int height)
{
    int mw = width, mh = height;
    getMinimumSize (mw, mh);
    if (width < mw)     width = mw;
    if (height < mh)    height = mh;

    if (customWidth != width || customHeight != height)
    {
        customWidth = width;
        customHeight = height;
        node.getBlockValueTree()
            .setProperty (Tags::width, customWidth, nullptr)
            .setProperty (Tags::height, customHeight, nullptr);
        
        if (displayMode == Small || displayMode == Compact)
        {
            displayModeValue.removeListener (this);
            displayModeValue.setValue (getDisplayModeKey (Normal));
            displayModeValue.addListener (this);
            displayMode = Normal;
        }
        
        setSize (customWidth, customHeight);
    }
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

Point<double> BlockComponent::getNodePosition() const noexcept
{
    Point<double> pos;
    node.getPosition (pos.x, pos.y);
    return pos;
}

void BlockComponent::updatePosition()
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
        x = x * (parent->getWidth()) - (getWidth() / 2);
        y = y * (parent->getHeight()) - (getHeight() / 2);
        node.setPosition (x, y);
    }
    else
    {
        node.getPosition (x, y);
    }

    setBounds ({ roundDoubleToInt (vertical ? x : y),
                 roundDoubleToInt (vertical ? y : x),
                 getWidth(),
                 getHeight() });
}

void BlockComponent::updatePins (bool force)
{
    int numInputs = 0, numOutputs = 0;
    const auto numPorts = node.getNumPorts();
    for (int i = 0; i < numPorts; ++i)
    {
        const Port port (node.getPort (i));
        if (PortType::Control == port.getType() || port.isHiddenOnBlock())
            continue;

        if (port.isInput())
            ++numInputs;
        else
            ++numOutputs;
    }

    if (force || numIns != numInputs || numOuts != numOutputs)
    {
        numIns = numInputs;
        numOuts = numOutputs;

        deleteAllPins();

        for (int i = 0; i < numPorts; ++i)
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

} // namespace Element

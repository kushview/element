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

#include "controllers/AppController.h"
#include "controllers/GuiController.h"
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

static bool elNodeIsAudioMixer (const Node& node)
{
    return node.getFormat().toString() == "Element"
        && node.getIdentifier().toString() == "element.audioMixer";
}

static bool elNodeIsMidiDevice (const Node& node)
{
    return node.getFormat().toString() == "Internal"
        && ( node.getIdentifier().toString() == "element.midiInputDevice" ||
             node.getIdentifier().toString() == "element.midiOutputDevice" );
}

static bool elNodeCanChangeIO (const Node& node)
{
    return !node.isIONode() 
        && !node.isGraph()
        && !elNodeIsAudioMixer (node)
        && !elNodeIsMidiDevice (node);
}

//=============================================================================

PortComponent::PortComponent (const Node& g, const Node& n,
                              const uint32 nid, const uint32 i, 
                              const bool dir, const PortType t, 
                              const bool v)
    : graph (g), node (n), nodeID (nid), port (i), 
      input (dir), type (t), vertical (v)
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

}

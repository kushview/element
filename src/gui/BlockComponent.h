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

#pragma once

#include "session/Node.h"

namespace Element {

class GraphEditorComponent;
class Node;

//=============================================================================

class PortComponent : public Component,
                      public SettableTooltipClient
{
public:
    PortComponent() = delete;
    PortComponent (const Node& graph, const Node& node,
                   const uint32 nodeId, const uint32 index, 
                   const bool isInput, const PortType type, 
                   const bool vertical);
    virtual ~PortComponent();

    bool isInput() const noexcept;
    uint32 getNodeId() const noexcept;
    uint32 getPortIndex() const noexcept;
    Colour getColor() const noexcept;

    void paint (Graphics& g) override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;

private:
    Node      graph;
    Node      node;
    uint32    nodeID    { 0 };
    uint32    port      { 0 };
    PortType  type      { PortType::Unknown };
    bool      input     { true };
    bool      vertical  { false };

    GraphEditorComponent* getGraphEditor() const noexcept;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PortComponent)
};

//=============================================================================

class BlockComponent : public Component
{
public:
    BlockComponent() = delete;
    BlockComponent (GraphEditorComponent& editor, const Node& node);
    virtual ~BlockComponent();

    uint32 getNodeId() const;

    void paint (Graphics&) override;
    void paintOverChildren (Graphics&) override;
    void resized() override;
    void mouseDown (const MouseEvent&) override;
    void mouseDrag (const MouseEvent&) override;
    void mouseUp (const MouseEvent&) override;
    
private:
    friend class GraphEditorComponent;
    friend class ConnectorComponent;
    friend class Impl; class Impl;
    std::unique_ptr<Impl> impl;
    void getPortPos (int, bool, float&, float&);
    void update (bool doPosition = true);
};

}

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
#include "gui/Buttons.h"

namespace Element {

class GraphEditorComponent;

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

class BlockComponent : public Component,
                       public Button::Listener,
                       private AsyncUpdater,
                       private Value::Listener
{
public:
    BlockComponent() = delete;
    BlockComponent (const Node& graph_, const Node& node_, const bool vertical_);
    ~BlockComponent() noexcept;

    void setNodePosition (const int x, const int y);
    void updatePosition();
    void makeEditorActive();
    void getPinPos (const int index, const bool isInput, float& x, float& y);
    void update (const bool doPosition = true);

    
    
    /** @internal */
    void buttonClicked (Button* b) override;

    /** @internal */
    bool hitTest (int x, int y) override;
    /** @internal */
    void mouseDown (const MouseEvent& e) override;
    /** @internal */
    void mouseDrag (const MouseEvent& e) override;
    /** @internal */
    void mouseUp (const MouseEvent& e) override;
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void paintOverChildren (Graphics& g) override;
    /** @internal */
    void resized() override;

private:
    friend class GraphEditorComponent;

    const uint32 filterID;
    Node graph;
    Node node;

    Value nodeEnabled;
    Value nodeName;

    int numInputs = 0, numOutputs = 0;
    int numIns = 0, numOuts = 0;

    double relativeX = 0.5f;
    double relativeY = 0.5f;

    int pinSize = 9;    
    Font font;
    
    Point<int> originalPos;
    bool selectionMouseDownResult = false;
    bool vertical = true;
    bool dragging = false;
    bool blockDrag = false;
    bool collapsed = false;

    SettingButton ioButton;
    PowerButton powerButton;
    SettingButton muteButton;

    OptionalScopedPointer<CallOutBox> ioBox;

    DropShadowEffect shadow;
    ScopedPointer<Component> embedded;

    void deleteAllPins();
    Rectangle<int> getOpenCloseBox() const;
    Rectangle<int> getBoxRectangle() const;
    GraphEditorComponent* getGraphPanel() const noexcept;

    void handleAsyncUpdate() override;
    void valueChanged (Value& value) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlockComponent);
};

}

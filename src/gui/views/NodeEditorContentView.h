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

#include "gui/ContentComponent.h"
#include "gui/widgets/NodeListComboBox.h"
#include "gui/Buttons.h"

namespace Element {

class NodeEditorContentView : public ContentView,
                              public ComboBox::Listener,
                              public Value::Listener
{
public:
    NodeEditorContentView();
    ~NodeEditorContentView();

    void getState (String&) override;
    void setState (const String&) override;

    void setSticky (bool shouldBeSticky);
    bool isSticky() const { return sticky; }

    Node getNode() const { return node; }
    void setNode (const Node&);

    void stabilizeContent() override;
    void resized() override;
    void paint (Graphics& g) override;

    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    void valueChanged (Value& value) override;

private:
    Node graph, node;
    Value nodeObjectValue;
    SignalConnection selectedNodeConnection;
    SignalConnection graphChangedConnection;
    SignalConnection sessionLoadedConnection;

    std::unique_ptr<Component> editor;
    NodeListComboBox nodesCombo;
    IconButton menuButton;
    bool sticky = false;
    void clearEditor();
    Component* createEmbededEditor();

    static void nodeMenuCallback (int, NodeEditorContentView*);
    void onGraphChanged();
    void onSessionLoaded();

    class NodeWatcher;
    std::unique_ptr<NodeWatcher> watcher;
};

} // namespace Element

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/content.hpp>
#include "ui/nodelistcombobox.hpp"
#include "ui/buttons.hpp"

namespace element {

class NodeEditorView : public ContentView,
                       public ComboBox::Listener,
                       public Value::Listener
{
public:
    NodeEditorView();
    ~NodeEditorView();

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
    boost::signals2::connection selectedNodeConnection,
        graphChangedConnection,
        sessionLoadedConnection;

    std::unique_ptr<Component> editor;
    NodeListComboBox nodesCombo;
    IconButton menuButton;
    bool sticky = false;
    void clearEditor();
    Component* createEmbededEditor();

    static void nodeMenuCallback (int, NodeEditorView*);
    void onGraphChanged();
    void onSessionLoaded();

    class NodeWatcher;
    std::unique_ptr<NodeWatcher> watcher;
};

} // namespace element

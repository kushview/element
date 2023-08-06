// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/services.hpp>
#include <element/session.hpp>
#include <element/ui/content.hpp>
#include <element/signals.hpp>

#include <element/engine.hpp>
#include "gui/TreeviewBase.h"

namespace element {

class SessionTreePanel : public juce::Component,
                         private juce::ValueTree::Listener
{
public:
    explicit SessionTreePanel();
    virtual ~SessionTreePanel();

    void refresh();

    void setSession (SessionPtr);
    void showNode (const Node& newNode);
    bool showingNode() const noexcept;

    SessionPtr session() const;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const MouseEvent& event) override;
    bool keyPressed (const KeyPress&) override;

private:
    friend class SessionNodeTreeItem;
    friend class SessionRootGraphTreeItem;
    
    class Panel;
    std::unique_ptr<Panel> panel;

    SessionPtr _session;
    ValueTree data;
    Node node;
    SignalConnection nodeSelectedConnection;

    bool ignoreActiveRootGraphSelectionHandler = false;
    void selectActiveRootGraph();

    TreeViewItem* findItemForNode (const Node& node) const;

    void onNodeSelected();

    friend class ValueTree;
    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property) override;
    void valueTreeChildAdded (ValueTree& parent, ValueTree& child) override;
    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int indexRomovedAt) override;
    void valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreeParentChanged (ValueTree& tree) override;
    void valueTreeRedirected (ValueTree& tree) override;
};

} // namespace element

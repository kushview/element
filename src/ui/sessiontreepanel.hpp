// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/services.hpp>
#include <element/session.hpp>
#include <element/ui/content.hpp>
#include <element/signals.hpp>
#include <element/engine.hpp>

#include "ui/treeviewbase.hpp"

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

    /** Returns the tree's expand/collapse state as an XML string.

        Nothing is written when every item is expanded or when the panel is
        showing a single node instead of the session.

        @param state Receives the state string, for persisting with the session.
    */
    void getState (juce::String& state) const;

    /** Restores expand/collapse state produced by getState().

        The state is applied the next time setSession() rebuilds the tree, so it
        can be handed over before the loaded session reaches the panel.

        @param state The state string; an empty string resets to the default.
    */
    void setState (const juce::String& state);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const MouseEvent& event) override;
    bool keyPressed (const KeyPress&) override;

private:
    friend class SessionNodeTreeItem;
    friend class SessionRootGraphTreeItem;
    friend class SessionRootTreeItem;

    class Panel;
    std::unique_ptr<Panel> panel;

    SessionPtr _session;
    ValueTree data;
    std::unique_ptr<juce::XmlElement> pendingOpenness;
    Node node;
    SignalConnection nodeSelectedConnection;

    bool ignoreActiveRootGraphSelectionHandler = false;
    void selectActiveRootGraph();

    TreeViewItem* findItemForNode (const Node& node) const;

    /** Moves a root graph and re-selects its tree item afterwards. */
    void moveRootGraph (const Node& graph, int newIndex);

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

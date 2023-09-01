// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/signals.hpp>

#include "ui/nodepropertypanel.hpp"
#include "ui/graphdisplayview.hpp"
#include "ui/grapheditorcomponent.hpp"

#define EL_VIEW_GRAPH_EDITOR "GraphEditorView"

namespace element {

class GraphEditorView : public GraphDisplayView,
                        public ChangeListener
{
public:
    GraphEditorView (const Node&);
    GraphEditorView();
    ~GraphEditorView();

    GraphEditor& editor() { return _editor; }

    void selectAllNodes();

    void didBecomeActive() override;
    void stabilizeContent() override;
    void willBeRemoved() override;

    ValueTree settings() const;
    void saveSettings();
    void restoreSettings();

    void paint (Graphics& g) override;
    bool keyPressed (const KeyPress& key) override;
    void changeListenerCallback (ChangeBroadcaster*) override;

protected:
    void graphDisplayResized (const Rectangle<int>& area) override;
    void graphNodeWillChange() override;
    void graphNodeChanged (const Node& g, const Node&) override;

private:
    Node node;
    GraphEditor _editor;

    SignalConnection nodeSelectedConnection,
        nodeRemovedConnection;
    void onNodeSelected();
    void onNodeRemoved (const Node&);
    void updateSizeInternal (const bool force = true);
    void init();
};

} // namespace element

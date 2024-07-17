// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/content.hpp>

#include "ui/midichannelselectcomponent.hpp"
#include "ui/nodemidiprogramcomponent.hpp"
#include "ui/nodelistcombobox.hpp"

namespace element {

class NodePropertiesView : public ContentView
{
public:
    NodePropertiesView();
    NodePropertiesView (const Node& node);
    ~NodePropertiesView();

    void setNode (const Node& node);
    Node node() const noexcept { return _node; }

    void setSticky (bool);
    bool isSticky() const noexcept { return sticky; }

    void stabilizeContent() override;
    void resized() override;
    void paint (Graphics& g) override;
    void paintOverChildren (Graphics& g) override;

    void getState (String& state) override;
    void setState (const String& state) override;

private:
    Node _node, _graph;
    boost::signals2::connection selectedNodeConnection,
        midiProgramChangedConnection,
        sessionLoadedConnection;

    class SignalLabel : public Label
    {
    public:
        SignalLabel() {}
        ~SignalLabel() {}

        inline void mouseDoubleClick (const MouseEvent& ev) override
        {
            if (onDoubleClicked)
                onDoubleClicked (ev);
        }

        std::function<void (const MouseEvent&)> onDoubleClicked;
    };

    NodeListComboBox combo;
    PropertyPanel props;
    NodeObjectSync nodeSync;
    IconButton menuButton;
    bool sticky = false;

    static void nodeMenuCallback (int, NodePropertiesView*);
    void onSessionLoaded();
    void updateProperties();
    void updateMidiProgram();
    void init();

    class NodeWatcher;
    std::unique_ptr<NodeWatcher> watcher;
};

} // namespace element

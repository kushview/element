// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/gui_basics.hpp>

namespace element {

class Node;
class NodeObjectSync;

/** A PropertyPanel which display node properties */
class NodePropertyPanel : public juce::PropertyPanel
{
public:
    NodePropertyPanel()
        : PropertyPanel()
    {
        initialize();
    }

    NodePropertyPanel (const Node& node)
        : PropertyPanel()
    {
        initialize();
        addProperties (node);
    }

    ~NodePropertyPanel() override = default;

    //==========================================================================
    /** Add properties from the given node. This will clear the panel before 
        adding

        @param node The node to get properties from
        @param extraSpace Extra space between properties (forwarded to juce::PropertyPanel)
     */
    void addProperties (const Node& node, int extraSpace = 0);

private:
    std::unique_ptr<NodeObjectSync> sync;
    void initialize();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodePropertyPanel)
};

} // namespace element

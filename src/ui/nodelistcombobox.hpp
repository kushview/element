// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/node.hpp>
#include <element/juce/gui_basics.hpp>

namespace element {

class NodeListComboBox : public juce::ComboBox
{
public:
    NodeListComboBox()
    {
        setTextWhenNoChoicesAvailable ("<empty>");
        setTextWhenNothingSelected ("<select node>");
    }

    using FilterFunction = std::function<bool (const Node&)>;
    inline static bool rejectIONodes (const Node& node) { return ! node.isIONode(); }
    virtual ~NodeListComboBox() {}

    void setFilter (FilterFunction fn)
    {
        _filter = fn;
    }

    void addNodes (const Node& parent,
                   juce::NotificationType notification = juce::sendNotificationAsync)
    {
        int lastIndex = getSelectedItemIndex();

        clear (notification);
        _nodes.clear();

        int idx = _offset;
        for (int i = 0; i < parent.getNumNodes(); ++i)
        {
            const auto node (parent.getNode (i));
            if (! filterNode (node))
                continue;
            _nodes.add (node);
            addItem (node.getDisplayName(), idx++);
        }

        if (isPositiveAndBelow (jmin (lastIndex, getNumItems() - _offset), getNumItems()))
            setSelectedItemIndex (lastIndex, notification);
    }

    const auto& nodes() const noexcept { return _nodes; }

    Node selectedNode() const noexcept
    {
        const auto idx = getSelectedId() - _offset;
        return idx >= 0 && idx < _nodes.size() ? _nodes[idx] : Node();
    }

    void selectNode (const Node& node,
                     NotificationType notification = sendNotificationAsync)
    {
        const int index = _nodes.indexOf (node);
        if (index != getSelectedItemIndex() && isPositiveAndBelow (index, getNumItems()))
            setSelectedItemIndex (index, notification);
    }

private:
    juce::Array<Node> _nodes;
    std::function<bool (const Node&)> _filter;
    const int _noneId = 1;
    int _offset = 2;

    bool filterNode (const Node& node) const noexcept
    {
        return _filter != nullptr ? _filter (node) : true;
    }
};

} // namespace element

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

    virtual ~NodeListComboBox() {}

    void addNodes (const Node& parent,
                   juce::NotificationType notification = juce::sendNotificationAsync)
    {
        int lastIndex = getSelectedItemIndex();

        clear (notification);
        _nodes.clear();

        for (int i = 0; i < parent.getNumNodes(); ++i)
        {
            const auto node (parent.getNode (i));
            _nodes.add (node);
            addItem (node.getDisplayName(), i + _offset);
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
    const int _noneId = 1;
    int _offset = 2;
};

} // namespace element

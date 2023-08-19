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

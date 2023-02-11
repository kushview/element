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

namespace element {

class NodeListComboBox : public ComboBox
{
public:
    NodeListComboBox()
    {
        setTextWhenNoChoicesAvailable ("Empty graph");
        setTextWhenNothingSelected ("Select node");
    }

    virtual ~NodeListComboBox() {}

    void addNodes (const Node& parent,
                   NotificationType notification = sendNotificationAsync)
    {
        int lastIndex = getSelectedItemIndex();

        clear (notification);
        for (int i = 0; i < parent.getNumNodes(); ++i)
        {
            const auto node (parent.getNode (i));
            addItem (node.getDisplayName(), i + 1);
        }

        if (isPositiveAndBelow (jmin (lastIndex, getNumItems() - 1), getNumItems()))
            setSelectedItemIndex (lastIndex, notification);
    }

    void selectNode (const Node& node,
                     NotificationType notification = sendNotificationAsync)
    {
        const auto graph (node.getParentGraph());
        const int index = graph.getNodesValueTree().indexOf (node.getValueTree());
        if (isPositiveAndBelow (index, getNumItems()))
            setSelectedItemIndex (index, notification);
    }
};

} // namespace element

/*
    This file is part of Element
    Copyright (C) 2021  Kushview, LLC.  All rights reserved.

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

#include "JuceHeader.h"

namespace Element {

class Node;

/** A PropertyPanel which display node properties */
class NodePropertyPanel : public PropertyPanel
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
    void initialize();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodePropertyPanel)
};

}

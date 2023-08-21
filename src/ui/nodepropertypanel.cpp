// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/node.hpp>

#include "ui/nodepropertypanel.hpp"
#include "ui/nodeproperties.hpp"

namespace element {

void NodePropertyPanel::initialize()
{
    setName ("NodePropertyPanel");
    setMessageWhenEmpty ("Empty node");
}

void NodePropertyPanel::addProperties (const Node& node, int extraSpace)
{
    sync.reset (nullptr);

    clear();

    if (node.isValid())
    {
        NodeProperties props (node, NodeProperties::General);
        PropertyPanel::addProperties (props, extraSpace);
        sync.reset (new NodeObjectSync (node));
    }
    else
    {
        setMessageWhenEmpty ("Invalid node");
    }

    refreshAll();
}

} // namespace element

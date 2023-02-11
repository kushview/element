
#include "gui/properties/NodePropertyPanel.h"
#include "gui/properties/NodeProperties.h"
#include <element/node.hpp>

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

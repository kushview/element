
#include "gui/properties/NodePropertyPanel.h"
#include "gui/properties/NodeProperties.h"
#include "session/Node.h"

namespace Element {

void NodePropertyPanel::initialize()
{
    setName ("NodePropertyPanel");
    setMessageWhenEmpty ("Empty node");
}

void NodePropertyPanel::addProperties (const Node& node, int extraSpace)
{
    clear();
    
    if (node.isValid())
    {
        NodeProperties props (node, NodeProperties::General);
        PropertyPanel::addProperties (props, extraSpace);
    }
    else
    {
        setMessageWhenEmpty ("Invalid node");
    }

    refreshAll();
}

}

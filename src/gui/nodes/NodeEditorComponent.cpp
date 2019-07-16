#include "gui/nodes/NodeEditorComponent.h"
#include "gui/PluginWindow.h"

namespace Element {

NodeEditorComponent::NodeEditorComponent (const Node& _node) noexcept
    : node (_node) { }
NodeEditorComponent::~NodeEditorComponent() {}

bool NodeEditorComponent::isRunningInPluginWindow() const
{
    return nullptr != findParentComponentOfClass<PluginWindow>(); 
}

}

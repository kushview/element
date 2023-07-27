// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/nodeeditor.hpp>
#include "gui/PluginWindow.h"

namespace element {

NodeEditor::NodeEditor (const Node& _node) noexcept
    : node (_node) {}
NodeEditor::~NodeEditor() {}

bool NodeEditor::isRunningInPluginWindow() const
{
    return nullptr != findParentComponentOfClass<PluginWindow>();
}

} // namespace element

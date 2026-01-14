// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/ui/nodeeditor.hpp>
#include "ui/pluginwindow.hpp"

namespace element {

Editor::Editor() {}
Editor::~Editor() {}

void Editor::setResizable (bool canResize)
{
    _resizable = canResize;
}

NodeEditor::NodeEditor (const Node& _node) noexcept
    : node (_node) {}
NodeEditor::~NodeEditor() {}

bool NodeEditor::isRunningInPluginWindow() const
{
    return nullptr != findParentComponentOfClass<PluginWindow>();
}

} // namespace element

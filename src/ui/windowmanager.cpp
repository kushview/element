// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/context.hpp>
#include <element/ui.hpp>
#include <element/processor.hpp>
#include <element/ui/commands.hpp>

#include "engine/graphnode.hpp"
#include "ui/nodeeditorfactory.hpp"
#include <element/ui/mainwindow.hpp>
#include "ui/windowmanager.hpp"

namespace element {

WindowManager::WindowManager (GuiService& g) : gui (g) {}

void WindowManager::onWindowClosed (Window* c)
{
    jassert (activeWindows.contains (c));
    c->setVisible (false);
    activeWindows.removeObject (c, true);
}

void WindowManager::closeOpenPluginWindowsFor (GraphNode& proc, const bool windowVisible)
{
    for (int i = 0; i < proc.getNumNodes(); ++i)
        if (auto node = proc.getNode (i))
            for (int j = activePluginWindows.size(); --j >= 0;)
                if (activePluginWindows.getUnchecked (j)->owner == node)
                {
                    deletePluginWindow (j, windowVisible);
                    break;
                }
}

void WindowManager::closeOpenPluginWindowsFor (Processor* const node, const bool windowVisible)
{
    if (! node)
        return;
    for (int i = activePluginWindows.size(); --i >= 0;)
        if (activePluginWindows.getUnchecked (i)->owner == node)
        {
            deletePluginWindow (i, windowVisible);
            break;
        }
}

void WindowManager::deletePluginWindow (PluginWindow* window, const bool windowVisible)
{
    jassert (activePluginWindows.contains (window));
    deletePluginWindow (activePluginWindows.indexOf (window), windowVisible);
}

void WindowManager::deletePluginWindow (const int index, const bool windowVisible)
{
    if (auto* window = activePluginWindows.getUnchecked (index))
    {
        window->node.setProperty (tags::windowVisible, windowVisible);
        window->removeKeyListener (gui.commands().getKeyMappings());
        window->removeKeyListener (gui.getKeyListener());
        activePluginWindows.remove (index);
    }
}

PluginWindow* WindowManager::createPluginWindowFor (const Node& n, Component* e)
{
    auto* window = activePluginWindows.add (new PluginWindow (gui, e, n));
    window->addKeyListener (gui.getKeyListener());
    window->addKeyListener (gui.commands().getKeyMappings());
    return window;
}

PluginWindow* WindowManager::createPluginWindowFor (const Node& node)
{
    NodeEditorFactory factory (gui);

    /** Try internal formats and custom GUIs. */
    if (auto e = factory.instantiate (node, NodeEditorPlacement::PluginWindow))
        return createPluginWindowFor (node, e.release());

    /** JUCE audio processor editor */
    if (auto editor = NodeEditorFactory::createAudioProcessorEditor (node))
        return createPluginWindowFor (node, editor.release());

    /** Try non-AudioProcessor plugin formats. */
    if (auto comp = NodeEditorFactory::createEditor (node))
        return createPluginWindowFor (node, comp.release());

    return nullptr;
}

} // namespace element

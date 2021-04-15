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

#include "Globals.h"
#include "controllers/GuiController.h"
#include "engine/NodeObject.h"
#include "engine///GraphNode.h"
#include "gui/NodeEditorFactory.h"
#include "gui/MainWindow.h"
#include "gui/WindowManager.h"

namespace Element {

WindowManager::WindowManager (GuiController& g) : gui(g) { }

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
                if (activePluginWindows.getUnchecked(j)->owner == node)
                    { deletePluginWindow (j, windowVisible); break; }
}
    
void WindowManager::closeOpenPluginWindowsFor (NodeObject* const node, const bool windowVisible)
{
    if (! node)
        return;
    for (int i = activePluginWindows.size(); --i >= 0;)
        if (activePluginWindows.getUnchecked(i)->owner == node)
            { deletePluginWindow (i, windowVisible); break; }
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
        window->node.setProperty (Tags::windowVisible, windowVisible);
        window->removeKeyListener (gui.commander().getKeyMappings());
        window->removeKeyListener (gui.getKeyListener());
        activePluginWindows.remove (index);
    }
}

PluginWindow* WindowManager::createPluginWindowFor (const Node& n, Component* e)
{
    auto* window = activePluginWindows.add (new PluginWindow (gui, e, n));
    window->addKeyListener (gui.getKeyListener());
    window->addKeyListener (gui.commander().getKeyMappings());
    return window;
}

PluginWindow* WindowManager::createPluginWindowFor (const Node& node)
{
    NodeEditorFactory factory (gui);
    if (auto e = factory.instantiate (node, NodeEditorPlacement::PluginWindow))
        return createPluginWindowFor (node, e.release());
    auto editor = NodeEditorFactory::createAudioProcessorEditor (node);
    return (editor != nullptr) ? createPluginWindowFor (node, editor.release()) : nullptr;
}

}


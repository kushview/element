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
    if (node.getIdentifier().toString() == EL_INTERNAL_ID_MIDI_PROGRAM_MAP)
    {
        auto* const pgced = new MidiProgramMapEditor (node);
        if (auto* object = dynamic_cast<MidiProgramMapNode*> (node.getGraphNode()))
            pgced->setSize (object->getWidth(), object->getHeight());

        return createPluginWindowFor (node, pgced);
    }
    else if (node.getIdentifier().toString() == EL_INTERNAL_ID_AUDIO_ROUTER)
    {
        auto* ared = new AudioRouterEditor (node);
        ared->setAutoResize (true);
        ared->adjustBoundsToMatrixSize (32);
        return createPluginWindowFor (node, ared);
    }
    else if (node.getIdentifier().toString() == EL_INTERNAL_ID_MIDI_ROUTER)
    {
        return createPluginWindowFor (node, new MidiRouterEditor (node));
    }
    else if (node.getIdentifier().toString() == EL_INTERNAL_ID_MIDI_MONITOR)
    {
        return createPluginWindowFor (node, new MidiMonitorNodeEditor (node));
    }
    else if (node.getIdentifier().toString() == EL_INTERNAL_ID_OSC_RECEIVER)
    {
        return createPluginWindowFor (node, new OSCReceiverNodeEditor (node));
    }
    else if (node.getIdentifier().toString() == EL_INTERNAL_ID_OSC_SENDER)
    {
        return createPluginWindowFor (node, new OSCSenderNodeEditor (node));
    }
    else if (node.getIdentifier().toString().contains ("element.volume"))
    {
        return createPluginWindowFor (node, new VolumeNodeEditor (node, gui));
    }
    else if (node.getIdentifier().toString() == EL_INTERNAL_ID_LUA)
    {
        return createPluginWindowFor (node, new LuaNodeEditor (node));
    }
    else if (node.getIdentifier().toString() == EL_INTERNAL_ID_SCRIPT)
    {
        return createPluginWindowFor (node, new ScriptNodeEditor (gui.getWorld().getScriptingEngine(), node));
    }
    
    NodeObjectPtr object = node.getGraphNode();
    AudioProcessor* proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
    if (! proc)
        return nullptr;
    if (! proc->hasEditor())
        return createPluginWindowFor (node, new GenericAudioProcessorEditor (proc));
    auto* editor = proc->createEditorIfNeeded();
    return (editor != nullptr) ? createPluginWindowFor (node, editor) : nullptr;
}

}


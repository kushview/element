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

#pragma once

#include "ElementApp.h"
#include "Commands.h"
#include "engine/nodes/BaseProcessor.h"
#include "engine/GraphProcessor.h"
#include "engine/GraphNode.h"
#include "engine/nodes/MidiMonitorNode.h"
#include "gui/Window.h"
#include "gui/PluginWindow.h"
#include "gui/nodes/AudioRouterEditor.h"
#include "gui/nodes/MidiMonitorNodeEditor.h"
#include "gui/nodes/MidiProgramMapEditor.h"
#include "gui/nodes/OSCReceiverNodeEditor.h"
#include "gui/nodes/OSCSenderNodeEditor.h"
#include "gui/nodes/VolumeNodeEditor.h"
#include "session/Node.h"

namespace Element {

class GuiController;

class WindowManager
{
public:
    WindowManager (GuiController& g);
    ~WindowManager()
    {
        closeAll();
    }

    inline void closeAll()
    {
        for (Window* w : activeWindows)
        {
            w->setVisible (false);
            w->removeFromDesktop();
        }
        
        activeWindows.clear (true);
        activeDialogs.clear (true);
        
        closeAllPluginWindows (true);
    }

    inline DialogWindow* findDialogByName (const String& name) const
    {
        for (auto* const d : activeDialogs)
            if (d->getName() == name)
                return d;
        return nullptr;
    }

    /** Show and manage a window or dialog

        The window manager takes ownership of the passed-in object
        The type needs to at least inherrit Compnent and WindowHook

        Another option would be to inherrit Component only, but provide
        yourself the required  Signal<void()>& signalClosed()  method 
     */
    inline void push (Window* window)
    {
        if (activeWindows.contains (window))
            return;
        activeWindows.add (window);
        window->addToDesktop();
        window->setVisible (true);
        window->signalClosed().connect (
            std::bind (&WindowManager::onWindowClosed, this, window));
    }

    /** Show and manage a dialog window */
        
    inline void push (DialogWindow* dialog, const bool alwaysOnTop = false)
    {
        jassert (nullptr != dialog);
        if (! activeDialogs.contains (dialog))
        {
            activeDialogs.add (dialog);
            dialog->setAlwaysOnTop (alwaysOnTop);
            dialog->addToDesktop();
            dialog->setVisible (true);
        }
    }

    // MARK: Plugin Windows
    
    inline int getNumPluginWindows() const { return activePluginWindows.size(); }
    
    inline PluginWindow* getPluginWindow (const int window) const
    {
        return activePluginWindows [window];
    }
    
    inline void closeOpenPluginWindowsFor (GraphProcessor& proc, const bool windowVisible)
    {
        for (int i = 0; i < proc.getNumNodes(); ++i)
            if (auto node = proc.getNode (i))
                for (int j = activePluginWindows.size(); --j >= 0;)
                    if (activePluginWindows.getUnchecked(j)->owner == node)
                        { deletePluginWindow (j, windowVisible); break; }
    }
    
    inline void closeOpenPluginWindowsFor (GraphNode* const node, const bool windowVisible)
    {
        if (! node)
            return;
        for (int i = activePluginWindows.size(); --i >= 0;)
            if (activePluginWindows.getUnchecked(i)->owner == node)
                { deletePluginWindow (i, windowVisible); break; }
    }
    
    inline void closeOpenPluginWindowsFor (const uint32 nodeId, const bool windowVisible)
    {
        for (int i = activePluginWindows.size(); --i >= 0;)
            if (activePluginWindows.getUnchecked(i)->owner->nodeId == nodeId)
                { deletePluginWindow (i, windowVisible); break; }
    }
    
    inline void closeOpenPluginWindowsFor (const Node& node, const bool windowVisible)
    {
        for (int i = activePluginWindows.size(); --i >= 0;)
            if (activePluginWindows.getUnchecked(i)->node == node)
                { deletePluginWindow (i, windowVisible); break; }
    }

    inline void closeAllPluginWindows (const bool windowVisible)
    {
        if (activePluginWindows.size() > 0)
        {
            for (int i = activePluginWindows.size(); --i >= 0;)
                deletePluginWindow (i, windowVisible);
            MessageManager::getInstance()->runDispatchLoopUntil (50);
        }
    }
    
    inline void closePluginWindow (PluginWindow* win)
    {
        deletePluginWindow (win, false);
    }
    
    inline PluginWindow* getPluginWindowFor (GraphNode* node)
    {
        for (auto* const window : activePluginWindows)
            if (window->owner == node)
               return window;
        return nullptr;
    }
    
    inline PluginWindow* getPluginWindowFor (const Node& node)
    {
        return getPluginWindowFor (node.getGraphNode());
    }

    inline PluginWindow* createPluginWindowFor (const Node& node)
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
            return createPluginWindowFor (node, new AudioRouterEditor (node));
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
        
        GraphNodePtr object = node.getGraphNode();
        AudioProcessor* proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
        if (! proc)
            return nullptr;
        if (! proc->hasEditor())
            return createPluginWindowFor (node, new GenericAudioProcessorEditor (proc));
        auto* editor = proc->createEditorIfNeeded();
        return (editor != nullptr) ? createPluginWindowFor (node, editor) : nullptr;
    }

private:
    GuiController& gui;
    OwnedArray<PluginWindow> activePluginWindows;
    OwnedArray<Window> activeWindows;
    OwnedArray<DialogWindow> activeDialogs;
    void onWindowClosed (Window* c);
    
    void deletePluginWindow (PluginWindow* window, const bool windowVisible);
    void deletePluginWindow (const int index, const bool windowVisible);
    PluginWindow* createPluginWindowFor (const Node& n, Component* e);
};

}

/*
    WindowManager.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "Commands.h"
#include "engine/GraphProcessor.h"
#include "engine/GraphNode.h"
#include "gui/Window.h"
#include "gui/PluginWindow.h"
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

    /** Show and manage a window or dialog

        The window manager takes ownership of the passed-in object
        The type needs to at least inherrit Compnent and WindowHook

        Another option would be to inherrit Component only, but provide
        yourself the required  Signal& signalClosed()  method */

    inline void push (Window* window)
    {
        activeWindows.addIfNotAlreadyThere (window);
        window->addToDesktop();
        window->setVisible (true);
        window->signalClosed().connect (
                    boost::bind (&WindowManager::onWindowClosed, this, window));
    }

    inline void push (DialogWindow* dialog)
    {
        if (! activeDialogs.contains (dialog))
        {
            activeDialogs.add (dialog);
            dialog->addToDesktop();
            dialog->setVisible (true);
        }
    }

    // MARK: Plugin Windows
    
    inline void closeOpenPluginWindowsFor (GraphProcessor& proc, const bool windowVisible)
    {
        for (int i = 0; i < proc.getNumNodes(); ++i)
            if (auto node = proc.getNode (i))
                for (int i = activePluginWindows.size(); --i >= 0;)
                    if (activePluginWindows.getUnchecked(i)->owner == node)
                        { deletePluginWindow (i, windowVisible); break; }
    }
    
    inline void closeOpenPluginWindowsFor (GraphNode* const node, const bool windowVisible)
    {
        for (int i = activePluginWindows.size(); --i >= 0;)
            if (activePluginWindows.getUnchecked(i)->owner == node)
                { deletePluginWindow (i, windowVisible); break; }
    }
    
    void closeOpenPluginWindowsFor (const uint32 nodeId, const bool windowVisible)
    {
        for (int i = activePluginWindows.size(); --i >= 0;)
            if (activePluginWindows.getUnchecked(i)->owner->nodeId == nodeId)
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
        for (int i = activePluginWindows.size(); --i >= 0;)
            if (activePluginWindows.getUnchecked(i)->owner == node)
               return activePluginWindows.getUnchecked(i);
        return nullptr;
    }
    
    inline PluginWindow* getPluginWindowFor (const Node& node)
    {
        return getPluginWindowFor (node.getGraphNode());
    }

    inline PluginWindow* createPluginWindowFor (const Node& node)
    {
        GraphNodePtr object = node.getGraphNode();
        AudioProcessor* proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
        if (! proc)
            return nullptr;
        if (! proc->hasEditor())
            return nullptr;
        
        auto* editor = proc->createEditorIfNeeded();
        return (editor != nullptr) ? createPluginWindowFor (node, editor) : nullptr;
    }
    
private:
    GuiController& gui;
    OwnedArray<PluginWindow> activePluginWindows;
    OwnedArray<Window> activeWindows;
    OwnedArray<DialogWindow> activeDialogs;
    void onWindowClosed (Window* c);
    
    inline void deletePluginWindow (PluginWindow* window, const bool windowVisible)
    {
        jassert (activePluginWindows.contains (window));
        deletePluginWindow (activePluginWindows.indexOf (window), windowVisible);
    }
    
    inline void deletePluginWindow (const int index, const bool windowVisible)
    {
        if (auto* window = activePluginWindows.getUnchecked (index))
        {
            window->node.setProperty ("windowVisible", windowVisible);
            activePluginWindows.remove (index);
        }
    }
    
    PluginWindow* createPluginWindowFor (const Node& n, Component* e);
};

}

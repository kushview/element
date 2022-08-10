/*
    This file is part of Element
    Copyright (C) 2019-2021  Kushview, LLC.  All rights reserved.
    - Author Michael Fisher <mfisher@kushview.net>

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
#include "gui/Window.h"
#include "gui/PluginWindow.h"
#include "session/Node.h"
#include "commands.hpp"

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
        return activePluginWindows[window];
    }

    void closeOpenPluginWindowsFor (GraphNode& proc, const bool windowVisible);

    void closeOpenPluginWindowsFor (NodeObject* const node, const bool windowVisible);

    inline void closeOpenPluginWindowsFor (const uint32 nodeId, const bool windowVisible)
    {
        for (int i = activePluginWindows.size(); --i >= 0;)
            if (activePluginWindows.getUnchecked (i)->owner->nodeId == nodeId)
            {
                deletePluginWindow (i, windowVisible);
                break;
            }
    }

    inline void closeOpenPluginWindowsFor (const Node& node, const bool windowVisible)
    {
        for (int i = activePluginWindows.size(); --i >= 0;)
            if (activePluginWindows.getUnchecked (i)->node == node)
            {
                deletePluginWindow (i, windowVisible);
                break;
            }
    }

    inline void closeAllPluginWindows (const bool windowVisible)
    {
        if (activePluginWindows.size() > 0)
        {
            for (int i = activePluginWindows.size(); --i >= 0;)
                deletePluginWindow (i, windowVisible);
           #if JUCE_MAC || JUCE_WINDOWS
            // needed?
            MessageManager::getInstance()->runDispatchLoopUntil (50);
           #endif
        }
    }

    inline void closePluginWindow (PluginWindow* win)
    {
        deletePluginWindow (win, false);
    }

    inline PluginWindow* getPluginWindowFor (NodeObject* node)
    {
        for (auto* const window : activePluginWindows)
            if (window->owner == node)
                return window;
        return nullptr;
    }

    inline PluginWindow* getPluginWindowFor (const Node& node)
    {
        return getPluginWindowFor (node.getObject());
    }

    PluginWindow* createPluginWindowFor (const Node& node);

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

} // namespace Element

/*
    WindowManager.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

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
        window->removeKeyListener (gui.getKeyListener());
        activePluginWindows.remove (index);
    }
}

PluginWindow* WindowManager::createPluginWindowFor (const Node& n, Component* e)
{
    auto* window = activePluginWindows.add (new PluginWindow (gui, e, n));
    window->addKeyListener (gui.getKeyListener());
    return window;
}

}


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


PluginWindow* WindowManager::createPluginWindowFor (const Node& n, Component* e)
{
    return activePluginWindows.add (new PluginWindow (gui, e, n));
}

}


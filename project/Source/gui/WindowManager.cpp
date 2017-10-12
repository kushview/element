/*
    WindowManager.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "gui/MainWindow.h"
#include "gui/WindowManager.h"

namespace Element {

WindowManager::WindowManager() { }

void WindowManager::onWindowClosed (Window* c)
{
    jassert (activeWindows.contains (c));
    c->setVisible (false);
    activeWindows.removeObject (c, true);
}

}

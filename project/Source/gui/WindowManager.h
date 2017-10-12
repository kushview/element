/*
    WindowManager.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "Commands.h"
#include "gui/Window.h"

namespace Element {
class WindowManager
{
public:
    WindowManager();
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
            dialog->runModalLoop();

            if (activeDialogs.contains (dialog))
                activeDialogs.remove (activeDialogs.indexOf (dialog), true);
        }
    }

private:
    OwnedArray<Window> activeWindows;
    OwnedArray<DialogWindow> activeDialogs;
    void onWindowClosed (Window* c);
};

}

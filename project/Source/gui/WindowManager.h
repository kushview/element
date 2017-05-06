/*
    WindowManager.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

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

#ifndef ELEMENT_WINDOW_MANAGER_H
#define ELEMENT_WINDOW_MANAGER_H

#include "ElementApp.h"
#include "Commands.h"
#include "gui/Window.h"

namespace Element {

    class GuiApp;

    class WindowManager
    {
    public:

        WindowManager (GuiApp& app);
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
        GuiApp& gui;
        
        void onWindowClosed (Window* c);
    };

}

#endif

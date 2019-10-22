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

#include "gui/MainWindow.h"
#include "gui/SystemTray.h"
#include "Globals.h"
#include "Commands.h"
#include "session/CommandManager.h"

namespace Element {

enum SystemTrayMouseAction
{
    ShowMenu = 0,
    ShowWindow
};

enum SystemTrayMenuResult
{
    ShowHideToggle = 1,
    ExitApplication
};

static MainWindow* getMainWindow()
{
    for (int i = DocumentWindow::getNumTopLevelWindows(); --i >= 0;)
        if (auto* const window = dynamic_cast<MainWindow*> (DocumentWindow::getTopLevelWindow(i)))
            return window;
    return nullptr;
}

SystemTray::SystemTray()
{
    Path path;
}

void SystemTray::mouseUp (const MouseEvent& ev)
{
    auto* window = getMainWindow();
    if (! window)
        return;
    auto* const cmd = &window->getWorld().getCommandManager();

    if (mouseUpAction == ShowMenu)
    {
        PopupMenu menu;
        menu.addCommandItem (cmd, Commands::toggleUserInterface, "Show/Hide");
        menu.addSeparator();
        menu.addCommandItem (cmd, Commands::quit, "Exit");
       #if JUCE_MAC
        showDropdownMenu (menu);
       #else
        menu.show();
       #endif
    }
    else 
    {
        window->setVisible (true);
        if (window->isMinimised())
            window->setMinimised (false);
        window->toFront (true);
    }

    mouseUpAction = -1;
}

void SystemTray::runMenu()
{
    auto* window = getMainWindow();
    if (! window)
        return;
    auto* const cmd = &window->getWorld().getCommandManager();

    PopupMenu menu;
    menu.addCommandItem (cmd, Commands::toggleUserInterface, "Show/Hide");
    menu.addSeparator();
    menu.addCommandItem (cmd, Commands::quit, "Exit");
   #if JUCE_MAC
    showDropdownMenu (menu);
   #else
    menu.show();
   #endif
}

void SystemTray::mouseDown (const MouseEvent& ev)
{
   #if JUCE_MAC
    ignoreUnused (ev);
    mouseUpAction = -1;
    runMenu();
   #else
    if (ev.mods.isPopupMenu())
    {
        mouseUpAction = ShowMenu;
    }
    else
    {
        mouseUpAction = ShowWindow;
    }
   #endif
}

}

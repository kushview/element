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
#include "session/commandmanager.hpp"
#include "commands.hpp"
#include <element/context.hpp>
#include "utils.hpp"

#include "BinaryData.h"

#define EL_USE_NEW_SYSTRAY_ICON 0
#define EL_SYSTRAY_MIN_SIZE 22

namespace element {

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
        if (auto* const window = dynamic_cast<MainWindow*> (DocumentWindow::getTopLevelWindow (i)))
            return window;
    return nullptr;
}

SystemTray* SystemTray::instance = nullptr;
SystemTray::SystemTray()
{
#if JUCE_MAC && EL_USE_NEW_SYSTRAY_ICON
    {
        const auto traySize = 22.f * 4;
        const float padding = 8.f;
        Image image (Image::ARGB, roundToInt (traySize), roundToInt (traySize), true);
        Graphics g (image);
        Icon icon (getIcons().falAtomAlt, Colours::black);
        icon.draw (g, { padding, padding, traySize - padding - padding, traySize - padding - padding }, false);
        instance->setIconImage (image, image);
    }
#else
    setIconImage (
        ImageCache::getFromMemory (BinaryData::ElementIcon_png, BinaryData::ElementIcon_pngSize),
        ImageCache::getFromMemory (BinaryData::ElementIconTemplate_png, BinaryData::ElementIcon_pngSize));
#endif
#if JUCE_LINUX
    setSize (EL_SYSTRAY_MIN_SIZE, EL_SYSTRAY_MIN_SIZE);
#endif
}

void SystemTray::setEnabled (bool enabled)
{
    if (element::Util::isRunningInWine())
        return;

    if (enabled)
    {
        if (instance == nullptr)
        {
            instance = new SystemTray();
            if (! instance->isOnDesktop())
                instance->addToDesktop (0);
        }
    }
    else
    {
        if (instance != nullptr)
        {
            if (instance->isOnDesktop())
                instance->removeFromDesktop();
            delete instance;
            instance = nullptr;
        }
    }
}

void SystemTray::mouseUp (const MouseEvent& ev)
{
    auto* window = getMainWindow();
    if (! window)
        return;
    auto* const cmd = &window->context().getCommandManager();

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
    auto* const cmd = &window->context().getCommandManager();

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

} // namespace element

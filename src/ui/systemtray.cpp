// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/ui/commands.hpp>
#include <element/ui/mainwindow.hpp>
#include <element/ui.hpp>
#include <element/context.hpp>

#include "ui/systemtray.hpp"
#include "ui/res.hpp"
#include "utils.hpp"

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
bool SystemTray::initialized = false;
static bool canUseSystemTray = true;

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
        ImageCache::getFromMemory (res::icon_png, res::icon_pngSize),
        ImageCache::getFromMemory (res::icontemplate_png, res::icontemplate_pngSize));
#endif
#if JUCE_LINUX
    setSize (EL_SYSTRAY_MIN_SIZE, EL_SYSTRAY_MIN_SIZE);
#endif
}

void SystemTray::init (GuiService& gui)
{
    if (initialized)
        return;

    initialized = true;
    canUseSystemTray = gui.getRunMode() == RunMode::Standalone;
}

void SystemTray::setEnabled (bool enabled)
{
    if (element::Util::isRunningInWine() || ! initialized || ! canUseSystemTray)
        return;

    if (enabled)
    {
        if (instance == nullptr)
            instance = new SystemTray();

        if (instance != nullptr && ! instance->isOnDesktop())
            instance->addToDesktop (0);
    }
    else
    {
        if (instance != nullptr)
        {
            if (instance->isOnDesktop())
                instance->removeFromDesktop();
        }
    }
}

void SystemTray::mouseUp (const MouseEvent& ev)
{
    auto* window = getMainWindow();
    if (! window)
        return;

    auto* const cmd = &window->services().find<UI>()->commands();
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

    auto* const cmd = &window->services().find<UI>()->commands();

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

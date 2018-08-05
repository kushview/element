
#include "gui/MainWindow.h"
#include "gui/SystemTray.h"

namespace Element {

enum SystemTrayMenuAction
{
    ShowMenu = 0,
    ShowWindow
};

enum SystemTrayMenuResult {
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

void SystemTray::mouseUp (const MouseEvent& ev)
{
    if (mouseUpAction == ShowMenu)
    {
        PopupMenu menu;
        menu.addItem (ShowHideToggle, "Show/Hide");
        menu.addSeparator();
        menu.addItem (ExitApplication, "Exit");
        const auto result = menu.show();
        if (result == ExitApplication)
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }
        else if (result == ShowHideToggle)
        {
            if (auto* const window = getMainWindow())
                window->setVisible (! window->isVisible());
        }
    }
    else if (mouseUpAction == ShowWindow)
    {
        if (auto* const window = getMainWindow())
        {
            window->setVisible (true);
            if (window->isMinimised())
                window->setMinimised (false);
            window->toFront (true);
        }
    }

    mouseUpAction = -1;
}

void SystemTray::mouseDown (const MouseEvent& ev)
{
    if (ev.mods.isRightButtonDown())
    {
        mouseUpAction = ShowMenu;
    }
    else
    {
        mouseUpAction = ShowWindow;
    }
}

}

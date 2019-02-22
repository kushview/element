#pragma once

#if EL_DOCKING

#include "ElementApp.h"

namespace Element {

class AppController;

class WorkspacePanel : public DockPanel
{
protected:
    WorkspacePanel() {}

public:
    virtual ~WorkspacePanel() {}

    virtual void initializeView (AppController&) { }
    virtual void didBecomeActive() { }
    virtual void stabilizeContent() { }

    void showPopupMenu() override
    {
        PopupMenu menu;
        menu.addItem (1, "Close Panel");
        menu.addItem (2, "Undock Panel");
        const auto result = menu.show();

        switch (result)
        {
            case 1: {
                close();
            } break;

            case 2: {
                undock();
            } break;

            default: break;
        }
    }
};

}

#endif

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ElementApp.h"

namespace element {

class SystemTray : public SystemTrayIconComponent,
                   public DeletedAtShutdown
{
public:
    ~SystemTray() = default;
    static SystemTray* getInstance() { return instance; }
    static void setEnabled (bool enabled);

    /** Returns true if a tray icon can be shown in this run mode and
        environment, regardless of whether the user has enabled it. */
    static bool isAvailable();

    void mouseDown (const MouseEvent&) override;
    void mouseUp (const MouseEvent&) override;

private:
    SystemTray();
    static SystemTray* instance;
    static bool initialized;
    int mouseUpAction = -1;
    void runMenu();

    friend class GuiService;
    static void init (GuiService&);
};

} // namespace element

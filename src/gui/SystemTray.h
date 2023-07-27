// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "ElementApp.h"

namespace element {

class SystemTray : public SystemTrayIconComponent,
                   public DeletedAtShutdown
{
public:
    ~SystemTray() = default;
    static SystemTray* getInstance() { return instance; }
    static void setEnabled (bool enabled);

    void mouseDown (const MouseEvent&) override;
    void mouseUp (const MouseEvent&) override;

private:
    SystemTray();
    static SystemTray* instance;
    int mouseUpAction = -1;
    void runMenu();
};

} // namespace element

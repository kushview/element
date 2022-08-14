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

#pragma once

#include "ElementApp.h"
#include "session/session.hpp"

namespace Element {

class AppController;
class Globals;
class MainMenu;

class MainWindow : public DocumentWindow,
                   public ChangeListener
{
public:
    MainWindow (Globals&);
    virtual ~MainWindow();
    void closeButtonPressed() override;
    void minimiseButtonPressed() override;

    void refreshMenu();
    Globals& getWorld() { return world; }
    AppController& getAppController();

    void changeListenerCallback (ChangeBroadcaster* source) override;
    void activeWindowStatusChanged() override;
    void refreshName();

private:
    Globals& world;
    std::unique_ptr<MainMenu> mainMenu;
    void nameChanged();
    void nameChangedSession();
    void nameChangedSingleGraph();
};

} // namespace Element

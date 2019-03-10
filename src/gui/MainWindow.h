/*
    MainWindow.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "session/Session.h"

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

}

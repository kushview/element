/*
    MainWindow.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"

namespace Element {

class Globals;
class MainMenu;

class MainWindow : public DocumentWindow
{
public:
    MainWindow (Globals&);
    virtual ~MainWindow();
    void closeButtonPressed();
    void refreshMenu();
    Globals& getWorld() { return world; }
    
private:
    Globals& world;
    ScopedPointer<MainMenu> mainMenu;
};

}

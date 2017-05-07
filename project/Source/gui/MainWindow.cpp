/*
    MainWindow.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "gui/ContentComponent.h"
#include "gui/MainMenu.h"
#include "gui/MainWindow.h"
#include "session/CommandManager.h"

namespace Element {
    MainWindow::MainWindow (CommandManager& cmd)
        : DocumentWindow ("Element", Colours::darkgrey, DocumentWindow::allButtons, false)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
        
        mainMenu = new MainMenu (*this, cmd);
        mainMenu->setupMenu();
        
        addKeyListener (cmd.getKeyMappings());
    }

    MainWindow::~MainWindow()
    {
        mainMenu = nullptr;
    }

    void MainWindow::closeButtonPressed()
    {
        JUCEApplication* app (JUCEApplication::getInstance());
        app->systemRequestedQuit();
    }

    void MainWindow::refreshMenu()
    {
        if (mainMenu)
            mainMenu->menuItemsChanged();
    }
}

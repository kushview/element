/*
    MainWindow.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "controllers/AppController.h"
#include "gui/ContentComponent.h"
#include "gui/MainMenu.h"
#include "gui/MainWindow.h"
#include "session/CommandManager.h"
#include "Globals.h"

namespace Element {

MainWindow::MainWindow (Globals& g)
    : DocumentWindow ("Element", Colours::darkgrey, DocumentWindow::allButtons, false),
        world (g)
{
    setUsingNativeTitleBar (true);
    setResizable (true, false);
    
    mainMenu = new MainMenu (*this, g.getCommandManager());
    mainMenu->setupMenu();
    
    addKeyListener (g.getCommandManager().getKeyMappings());
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

AppController& MainWindow::getAppController()
{
    jassert(nullptr != dynamic_cast<ContentComponent*> (getContentComponent()));
    return (dynamic_cast<ContentComponent*> (getContentComponent()))
        ->getAppController();
}

}

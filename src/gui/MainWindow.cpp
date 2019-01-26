/*
    MainWindow.cpp - This file is part of Element
    Copyright (C) 2016-2019 Kushview, LLC.  All rights reserved.
*/

#include "controllers/AppController.h"
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
    mainMenu = new MainMenu (*this, g.getCommandManager());
    mainMenu->setupMenu();
    nameChanged (g.getSession()->getName());
    g.getSession()->addChangeListener (this);
    addKeyListener (g.getCommandManager().getKeyMappings());
    setUsingNativeTitleBar (true);
    setResizable (true, false);
}

MainWindow::~MainWindow()
{
    world.getSession()->removeChangeListener (this);
    mainMenu = nullptr;
}

void MainWindow::changeListenerCallback (ChangeBroadcaster*)
{
    refreshName();
    // refreshMenu();
}

void MainWindow::refreshName()
{
    nameChanged (world.getSession()->getName());
}

void MainWindow::nameChanged (const String& value)
{
    const auto newName = value.trim();
    if (newName != sessionName)
       sessionName = newName;

    String title = "Element";
   #ifdef EL_FREE
    title << " FREE";
   #endif
    if (sessionName.isNotEmpty())
        title << " - " << sessionName;
    
    setName (title);
}

void MainWindow::closeButtonPressed()
{
    JUCEApplication* app (JUCEApplication::getInstance());
    app->systemRequestedQuit();
}

void MainWindow::activeWindowStatusChanged()
{
    if (nullptr == getContentComponent())
        return;
    auto& app (getAppController());
    app.checkForegroundStatus();
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

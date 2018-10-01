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

    nameChanged (g.getSession()->getName());
    g.getSession()->addChangeListener (this);
    addKeyListener (g.getCommandManager().getKeyMappings());
}

MainWindow::~MainWindow()
{
    world.getSession()->removeChangeListener (this);
    mainMenu = nullptr;
}

void MainWindow::changeListenerCallback (ChangeBroadcaster*)
{
    nameChanged (world.getSession()->getName());
}

void MainWindow::nameChanged (const String& value)
{
    const auto newName = value.trim();
    if (newName != sessionName)
       sessionName = newName;

    String title = "Element";
    if (sessionName.isNotEmpty())
        title << " - " << sessionName;
    setName (title);
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

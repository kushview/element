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
#include "Utils.h"

namespace Element {

MainWindow::MainWindow (Globals& g)
    : DocumentWindow ("Element", Colours::darkgrey, DocumentWindow::allButtons, false),
      world (g)
{
    mainMenu.reset (new MainMenu (*this, g.getCommandManager()));
    mainMenu->setupMenu();
    nameChanged();
    g.getSession()->addChangeListener (this);
    addKeyListener (g.getCommandManager().getKeyMappings());
    setUsingNativeTitleBar (true);
    setResizable (true, false);
}

MainWindow::~MainWindow()
{
    world.getSession()->removeChangeListener (this);
    mainMenu.reset();
}

void MainWindow::changeListenerCallback (ChangeBroadcaster*)
{
    refreshName();
    // refreshMenu();
}

void MainWindow::refreshName()
{
    nameChanged();
}

void MainWindow::nameChanged()
{
    String title = Util::appName();

    if (auto session = world.getSession())
    {
        const auto sessName = session->getName().trim();
        const auto graphName = session->getCurrentGraph().getName();
       #if defined (EL_PRO)
        if (sessName.isNotEmpty())
            title << " - " << sessName;
        if (graphName.isNotEmpty())
            title << ": " << graphName;
       #else
        if (graphName.isNotEmpty())
            title << " - " << graphName;
       #endif
    }

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

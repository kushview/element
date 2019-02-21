/*
    MainWindow.cpp - This file is part of Element
    Copyright (C) 2016-2019 Kushview, LLC.  All rights reserved.
*/

#include "controllers/GraphController.h"
#include "controllers/SessionController.h"
#include "gui/ContentComponent.h"
#include "gui/MainMenu.h"
#include "gui/MainWindow.h"
#include "session/CommandManager.h"
#include "Globals.h"
#include "Utils.h"

namespace Element {

MainWindow::MainWindow (Globals& g)
    : DocumentWindow (Util::appName(), Colours::darkgrey, DocumentWindow::allButtons, false),
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
    String sessionName, graphName;
    if (auto session = world.getSession())
    {
        sessionName     = session->getName().trim();
        graphName       = session->getCurrentGraph().getName().trim();
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

    if (nullptr != dynamic_cast<ContentComponent*> (getContentComponent()))
    {
       #if defined (EL_PRO)
        if (auto* const sc = getAppController().findChild<SessionController>())
        {
            ignoreUnused (sc); // noop
        }
       #else
        if (auto* const gc = getAppController().findChild<GraphController>())
        {
            const auto file = gc->getGraphFile();

            if (graphName.isEmpty())
            {
                if (file.existsAsFile())
                    title << " - " << file.getFileNameWithoutExtension();
            }
            else
            {
                // SAVEME: Shows file name if title isn't empty and file exists
                // if (file.existsAsFile())
                //     title << " (" << file.getFileName() << ")";
            }
        }
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

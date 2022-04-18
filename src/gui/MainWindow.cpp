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

#include "controllers/GraphController.h"
#include "controllers/SessionController.h"
#include "gui/ContentComponent.h"
#include "gui/MainMenu.h"
#include "gui/MainWindow.h"
#include "session/CommandManager.h"
#include "Commands.h"
#include "Globals.h"
#include "Settings.h"
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
}

void MainWindow::refreshName()
{
    nameChanged();
}

void MainWindow::nameChanged()
{
#if defined(EL_PRO)
    nameChangedSession();
#else
    nameChangedSingleGraph();
#endif
}

void MainWindow::nameChangedSession()
{
    String title = Util::appName();

    auto session = world.getSession();
    SessionController* controller = nullptr;
    if (auto* cc = dynamic_cast<ContentComponent*> (getContentComponent()))
        controller = getAppController().findChild<SessionController>();

    if (nullptr == session || nullptr == controller)
    {
        setName (title);
        return;
    }

    auto sessionName = session->getName().trim();
    auto graphName = session->getCurrentGraph().getName().trim();
    if (sessionName.isEmpty())
    {
        const auto file = controller->getSessionFile();
        if (file.existsAsFile())
            sessionName = file.getFileNameWithoutExtension();
    }

    if (sessionName.isEmpty())
        sessionName = "Untitled Session";
    if (graphName.isEmpty())
        graphName = "Untitled Graph";

    title << " - " << sessionName << ": " << graphName;
    setName (title);
}

void MainWindow::nameChangedSingleGraph()
{
    String title = Util::appName();
    String sessionName, graphName;
    if (auto session = world.getSession())
    {
        sessionName = session->getName().trim();
        graphName = session->getCurrentGraph().getName().trim();
        if (graphName.isNotEmpty())
            title << " - " << graphName;
    }

    if (nullptr != dynamic_cast<ContentComponent*> (getContentComponent()))
    {
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
    }

    setName (title);
}

void MainWindow::closeButtonPressed()
{
    JUCEApplication* app (JUCEApplication::getInstance());
    app->systemRequestedQuit();
}

void MainWindow::minimiseButtonPressed()
{
    if (world.getSettings().isSystrayEnabled())
        world.getCommandManager().invokeDirectly (Commands::toggleUserInterface, true);
    else
        DocumentWindow::minimiseButtonPressed();
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
    jassert (nullptr != dynamic_cast<ContentComponent*> (getContentComponent()));
    return (dynamic_cast<ContentComponent*> (getContentComponent()))
        ->getAppController();
}

} // namespace Element

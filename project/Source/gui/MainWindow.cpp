/*
    MainWindow.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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

#include "ContentComponent.h"
#include "MainMenu.h"
#include "MainWindow.h"

namespace Element {
namespace Gui {


    MainWindow::MainWindow (GuiApp& gui_)
        : DocumentWindow ("Element", Colours::darkgrey, DocumentWindow::allButtons, false),
          gui (gui_)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);

        mainMenu = new MainMenu (*this);
        gui.commander().registerAllCommandsForTarget (mainMenu);
        gui.commander().setFirstCommandTarget (mainMenu);
        addKeyListener (gui.commander().getKeyMappings());
        mainMenu->setupMenu();
    }

    MainWindow::~MainWindow()
    {
        mainMenu = nullptr;
    }

    void
    MainWindow::closeButtonPressed()
    {
        JUCEApplication* app (JUCEApplication::getInstance());
        app->systemRequestedQuit();
    }


}} /* namespace Element::Gui */

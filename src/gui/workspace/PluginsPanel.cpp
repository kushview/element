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

#include "controllers/AppController.h"
#include "gui/views/PluginsPanelView.h"
#include "gui/workspace/PluginsPanel.h"
#include "globals.hpp"
#include "session/pluginmanager.hpp"

namespace Element {

PluginsPanel::PluginsPanel() {}

void PluginsPanel::initializeView (AppController& app)
{
    if (view != nullptr)
        return;
    view.reset (new PluginsPanelView (app.getWorld().getPluginManager()));
    addAndMakeVisible (view.get());
}

void PluginsPanel::didBecomeActive() {}
void PluginsPanel::stabilizeContent() {}

void PluginsPanel::resized()
{
    if (view)
        view->setBounds (getLocalBounds());
}

} // namespace Element

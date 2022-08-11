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

#include "controllers/PresetsController.h"
#include "controllers/GuiController.h"
#include "gui/ContentComponent.h"
#include "session/Session.h"
#include "session/Presets.h"
#include "globals.hpp"
#include "datapath.hpp"

namespace Element {

struct PresetsController::Pimpl
{
    Pimpl() {}
    ~Pimpl() {}

    void refresh()
    {
    }
};

PresetsController::PresetsController()
{
    pimpl.reset (new Pimpl());
}

PresetsController::~PresetsController()
{
    pimpl.reset (nullptr);
}

void PresetsController::activate()
{
}

void PresetsController::deactivate()
{
}

void PresetsController::refresh()
{
    getWorld().getPresetCollection().refresh();
}

void PresetsController::add (const Node& node, const String& presetName)
{
    const DataPath path;
    if (! node.savePresetTo (path, presetName))
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                          "Preset",
                                          "Could not save preset");
    }
    else
    {
        getWorld().getPresetCollection().refresh();
    }

    if (auto* gui = findSibling<GuiController>())
        if (auto* cc = gui->getContentComponent())
            cc->stabilize (true);
}

} // namespace Element

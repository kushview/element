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

#include <element/ui.hpp>
#include <element/ui/content.hpp>
#include <element/session.hpp>
#include "session/presetmanager.hpp"
#include <element/context.hpp>

#include "datapath.hpp"
#include "services/presetservice.hpp"

using juce::String;

namespace element {

struct PresetService::Impl
{
    Impl() {}
    ~Impl() {}

    void refresh()
    {
    }
};

PresetService::PresetService()
{
    impl.reset (new Impl());
}

PresetService::~PresetService()
{
    impl.reset (nullptr);
}

void PresetService::activate()
{
}

void PresetService::deactivate()
{
}

void PresetService::refresh()
{
    context().presets().refresh();
}

void PresetService::add (const Node& node, const String& presetName)
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
        context().presets().refresh();
    }

    if (auto* ui = sibling<UI>())
        ui->stabilizeContent();
}

} // namespace element

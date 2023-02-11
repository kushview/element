/*
    This file is part of Element.
    Copyright (C) 2020-2021 Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "services.hpp"
#include "scripting.hpp"
#include "gui/views/LuaConsoleView.h"
#include <element/context.hpp>

namespace element {

LuaConsoleView::~LuaConsoleView()
{
    if (log != nullptr)
    {
        log->removeListener (this);
        log = nullptr;
    }
}

void LuaConsoleView::initializeView (ServiceManager& app)
{
    auto& se = app.getWorld().getScriptingEngine();
    sol::state_view view (se.getLuaState());
    console.setEnvironment (
        sol::environment (view, sol::create, view.globals()));

    log = &app.getWorld().getLog();
    log->addListener (this);

    String buffer;
    for (const auto& line : log->getHistory())
        buffer << line << juce::newLine;
    console.addText (buffer.trimEnd(), false);
}

} // namespace element

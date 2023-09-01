// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/context.hpp>
#include <element/services.hpp>

#include "scripting.hpp"
#include "ui/luaconsoleview.hpp"

namespace element {

LuaConsoleView::~LuaConsoleView()
{
    if (log != nullptr)
    {
        log->removeListener (this);
        log = nullptr;
    }
}

void LuaConsoleView::initializeView (Services& app)
{
    auto& se = app.context().scripting();
    sol::state_view view (se.getLuaState());
    console.setEnvironment (
        sol::environment (view, sol::create, view.globals()));

    log = &app.context().logger();
    log->addListener (this);

    String buffer;
    for (const auto& line : log->getHistory())
        buffer << line << juce::newLine;
    console.addText (buffer.trimEnd(), false);
}

} // namespace element

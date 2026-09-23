// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

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
    console.initialize (app.context().scripting());

    log = &app.context().logger();
    log->addListener (this);

    String buffer;
    for (const auto& line : log->getHistory())
        buffer << line << juce::newLine;
    console.addText (buffer.trimEnd(), false);
}

void LuaConsoleView::messageLogged (const String& msg)
{
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<LuaConsoleView> (this), msg]() {
        if (safe != nullptr)
            safe->console.addText (msg, false);
    });
}

} // namespace element

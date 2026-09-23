// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/ui/content.hpp>
#include "ui/luaconsole.hpp"
#include "log.hpp"

#define EL_VIEW_CONSOLE "LuaConsoleView"
/** Misspelled view name persisted to settings by older builds. */
#define EL_VIEW_CONSOLE_LEGACY "LuaConsoleViw"

namespace element {

class Services;

class LuaConsoleView : public ContentView,
                       public Log::Listener
{
public:
    LuaConsoleView()
    {
        setName (EL_VIEW_CONSOLE);
        addAndMakeVisible (console);
    }

    ~LuaConsoleView();

    void initializeView (Services&) override;
    void didBecomeActive() override
    {
        if (isShowing() || isOnDesktop())
            console.grabKeyboardFocus();
    }

    void resized() override
    {
        console.setBounds (getLocalBounds().reduced (2));
    }

    /** Called from whichever thread logs the message, inside the Log's lock. */
    void messageLogged (const String& msg) override;

private:
    LuaConsole console;
    Log* log = nullptr;
};

} // namespace element

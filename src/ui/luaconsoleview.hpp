// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/ui/content.hpp>
#include "ui/luaconsole.hpp"
#include "log.hpp"

#define EL_VIEW_CONSOLE "LuaConsoleViw"

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

    void messageLogged (const String& msg) override
    {
        console.addText (msg, false);
    }

private:
    LuaConsole console;
    Log* log = nullptr;
};

} // namespace element

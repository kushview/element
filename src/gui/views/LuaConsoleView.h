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

#pragma once

#include <element/ui/content.hpp>
#include "gui/widgets/LuaConsole.h"
#include "log.hpp"

namespace element {

class ServiceManager;

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

    void initializeView (ServiceManager&) override;
    void didBecomeActive() override
    {
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

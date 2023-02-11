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

#pragma once

#include "ElementApp.h"
#include "services/guiservice.hpp"
#include "gui/LookAndFeel.h"
#include "session/commandmanager.hpp"
#include "commands.hpp"
#include <element/context.hpp>
#include "utils.hpp"

namespace element {

class AboutComponent : public Component
{
public:
    AboutComponent();
    void resized() override;
    void paint (Graphics& g) override;

private:
    Label titleLabel { "title", Util::appName().toUpperCase() },
        versionLabel { "version" },
        copyrightLabel { "copyright", String (CharPointer_UTF8 ("\xc2\xa9")) + String (" 2019 Kushview, LLC.") };
    HyperlinkButton aboutButton { "About Us", URL ("https://kushview.net") };
    Rectangle<float> elementLogoBounds;
    std::unique_ptr<Drawable> elementLogo;
    TabbedComponent tabs { TabbedButtonBar::TabsAtTop };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutComponent)
};

class AboutDialog : public DialogWindow
{
public:
    AboutDialog (GuiService& g)
        : DialogWindow ("About Element",
                        g.getLookAndFeel().findColour (DocumentWindow::backgroundColourId),
                        true,
                        false),
          gui (g)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (new AboutComponent(), true);
    }

    void closeButtonPressed() override
    {
        gui.getWorld().getCommandManager().invokeDirectly (
            Commands::showAbout, true);
    }

private:
    GuiService& gui;
};

} // namespace element

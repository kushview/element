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

#include "gui/ContentComponent.h"
#include "gui/LookAndFeel.h"

namespace Element {

class EmptyContentView : public ContentView
{
public:
    EmptyContentView()
    {
        setName ("EmptyView");
    }

    inline void paint (Graphics& g) override
    {
        g.fillAll (LookAndFeel::contentBackgroundColor);
        g.setColour (LookAndFeel::textColor);
        g.setFont (16.f);

#if JUCE_MAC
        const String msg ("Session is empty.\nPress Shift+Cmd+N to add a graph.");
#else
        const String msg ("Session is empty.\nPress Shift+Ctl+N to add a graph.");
#endif
        g.drawFittedText (msg, 0, 0, getWidth(), getHeight(), Justification::centred, 2);
    }
};

} // namespace Element

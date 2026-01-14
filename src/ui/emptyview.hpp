// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/ui/content.hpp>
#include <element/ui/style.hpp>

namespace element {

class EmptyContentView : public ContentView
{
public:
    EmptyContentView()
    {
        setName ("EmptyView");
    }

    inline void paint (Graphics& g) override
    {
        g.fillAll (Colors::contentBackgroundColor);
        g.setColour (Colors::textColor);
        g.setFont (16.f);

#if JUCE_MAC
        const String msg ("Session is empty.\nPress Shift+Cmd+N to add a graph.");
#else
        const String msg ("Session is empty.\nPress Shift+Ctl+N to add a graph.");
#endif
        g.drawFittedText (msg, 0, 0, getWidth(), getHeight(), Justification::centred, 2);
    }
};

} // namespace element

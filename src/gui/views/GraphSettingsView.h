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

#include "gui/Buttons.h"
#include <element/ui/content.hpp>

namespace element {

class GraphPropertyPanel;
class GraphSettingsView : public ContentView,
                          public Button::Listener,
                          private Value::Listener
{
public:
    GraphSettingsView();
    ~GraphSettingsView();

    void setPropertyPanelHeaderVisible (bool);
    void setGraphButtonVisible (bool isVisible);
    void setUpdateOnActiveGraphChange (bool shouldUpdate);

    void resized() override;
    void didBecomeActive() override;
    void stabilizeContent() override;
    void buttonClicked (Button*) override;

private:
    ScopedPointer<GraphPropertyPanel> props;
    GraphButton graphButton;
    Value activeGraphIndex;
    bool updateWhenActiveGraphChanges = false;

    void valueChanged (Value& value) override;
};

} // namespace element

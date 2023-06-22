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

#include "gui/GuiCommon.h"
#include "gui/views/SessionSettingsView.h"
#include "session/commandmanager.hpp"

namespace element {
typedef Array<PropertyComponent*> PropertyArray;

class SessionPropertyPanel : public PropertyPanel
{
public:
    SessionPropertyPanel() {}
    ~SessionPropertyPanel()
    {
        clear();
    }

    void setSession (SessionPtr newSession)
    {
        clear();
        session = newSession;
        if (session)
        {
            PropertyArray props;
            getSessionProperties (props, session);
            addSection ("Session Settings", props);
        }
    }

private:
    SessionPtr session;
    static void getSessionProperties (PropertyArray& props, SessionPtr s)
    {
        props.add (new TextPropertyComponent (s->getPropertyAsValue (tags::name),
                                              "Name",
                                              256,
                                              false));
        props.add (new SliderPropertyComponent (s->getPropertyAsValue (tags::tempo),
                                                "Tempo",
                                                EL_TEMPO_MIN,
                                                EL_TEMPO_MAX,
                                                1));
        props.add (new TextPropertyComponent (s->getPropertyAsValue (tags::notes),
                                              "Notes",
                                              512,
                                              true));
    }
};

SessionContentView::SessionContentView()
{
    setName ("SessionSettings");
    addAndMakeVisible (props = new SessionPropertyPanel());
    setEscapeTriggersClose (true);
    addAndMakeVisible (graphButton);
    graphButton.setTooltip ("Show graph editor");
    graphButton.onClick = [this]() {
        // FIXME: Commands
        // if (auto* g = ViewHelpers::getGlobals (this))
        //     g->getCommandManager().invokeDirectly (Commands::showGraphEditor, true);
    };
}

SessionContentView::~SessionContentView()
{
    graphButton.onClick = nullptr;
}

void SessionContentView::didBecomeActive()
{
    grabKeyboardFocus();
    props->setSession (ViewHelpers::getSession (this));
    resized();
}

void SessionContentView::paint (Graphics& g)
{
    g.fillAll (LookAndFeel::contentBackgroundColor);
}

void SessionContentView::resized()
{
    props->setBounds (getLocalBounds().reduced (2));
    const int configButtonSize = 14;
    graphButton.setBounds (getWidth() - configButtonSize - 4, 4, configButtonSize, configButtonSize);
}
} // namespace element

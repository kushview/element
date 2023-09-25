// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/commands.hpp>

#include "ui/guicommon.hpp"
#include "ui/sessionsettingsview.hpp"

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
    setName (EL_VIEW_SESSION_SETTINGS);
    props = std::make_unique<SessionPropertyPanel>();
    addAndMakeVisible (props.get());
    setEscapeTriggersClose (true);
}

SessionContentView::~SessionContentView()
{
}

void SessionContentView::didBecomeActive()
{
    grabKeyboardFocus();
    props->setSession (ViewHelpers::getSession (this));
    resized();
}

void SessionContentView::paint (Graphics& g)
{
    g.fillAll (Colors::contentBackgroundColor);
}

void SessionContentView::resized()
{
    props->setBounds (getLocalBounds().reduced (2));
    const int configButtonSize = 14;
}
} // namespace element

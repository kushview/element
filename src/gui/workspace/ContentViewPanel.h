#pragma once

#include "gui/ContentComponent.h"
#include "gui/workspace/WorkspacePanel.h"

namespace Element {

template<class ViewType>
class ContentViewPanel :  public WorkspacePanel
{
public:
    ContentViewPanel() : WorkspacePanel()
    {
        addAndMakeVisible (view);
    }

    ~ContentViewPanel() { }
    
    void initializeView (AppController& app) override   { view.initializeView (app); }
    void didBecomeActive() override                     { view.didBecomeActive(); }
    void stabilizeContent() override                    { view.stabilizeContent(); }

    void resized() override
    {
        view.setBounds (getLocalBounds());
    }

protected:
    ViewType view;
};

}

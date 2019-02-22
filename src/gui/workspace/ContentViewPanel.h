#pragma once

#include "gui/ContentComponent.h"
#include "gui/workspace/WorkspacePanel.h"
#include "gui/views/SessionSettingsView.h"
#include "gui/views/NodeChannelStripView.h"
#include "gui/views/NodeEditorContentView.h"

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


class SessionSettingsPanel : public ContentViewPanel<SessionSettingsView>
{
public:
    SessionSettingsPanel() { setName ("Session"); }
    ~SessionSettingsPanel() = default;
};

class NodeChannelStripPanel : public ContentViewPanel<NodeChannelStripView>
{
public:
    NodeChannelStripPanel() { setName ("Strip"); }
    ~NodeChannelStripPanel() = default;
};

class NodeEditorPanel : public ContentViewPanel<NodeEditorContentView>
{
public:
    NodeEditorPanel() { setName ("Node"); }
    ~NodeEditorPanel() = default;
};

}

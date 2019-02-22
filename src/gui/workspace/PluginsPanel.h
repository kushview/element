#pragma once

#include "gui/workspace/WorkspacePanel.h"

namespace Element {

class PluginsPanelView;

class PluginsPanel : public WorkspacePanel
{
public:
    PluginsPanel();
    ~PluginsPanel() = default;

    void initializeView (AppController&) override;
    void didBecomeActive() override;
    void stabilizeContent() override;

    void resized() override;

private:
    std::unique_ptr<PluginsPanelView> view;
};

}

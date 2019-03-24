#pragma once

#include "controllers/AppController.h"

namespace Element {

class ContentComponent;

class WorkspacesController final : public AppController::Child
{
public:
    WorkspacesController() = default;
    ~WorkspacesController() = default;
    
    void activate() override;
    void deactivate() override;
    void saveSettings() override;

protected:
    bool handleMessage (const AppMessage& msg) override;
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands (Array<CommandID>&) override;
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override;
    bool perform (const InvocationInfo&) override;

private:
    Component::SafePointer<ContentComponent> content;
    void saveCurrentWorkspace();
    void saveCurrentAndLoadWorkspace (const String& name);
};

}

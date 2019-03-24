#include "Common.h"
#include "controllers/WorkspacesController.h"
#include "gui/ContentComponent.h"
#include "gui/Workspace.h"

namespace Element {

void WorkspacesController::activate()
{
    content = findSibling<GuiController>()->getContentComponent();
}

void WorkspacesController::deactivate()
{
    content = nullptr;
}

void WorkspacesController::saveSettings()
{
    saveCurrentWorkspace();
}

bool WorkspacesController::handleMessage (const AppMessage& msg)
{
    bool handled = true;
    if (const auto* wofm = dynamic_cast<const WorkspaceOpenFileMessage*> (&msg))
    {
        saveCurrentWorkspace();
        const auto state = WorkspaceState::fromFile (wofm->file, true);
        content->applyWorkspaceState (state);
    }
    else
    {
        handled = false;
    }

    return handled;
}

ApplicationCommandTarget* WorkspacesController::getNextCommandTarget() { return findFirstTargetParentComponent(); }

void WorkspacesController::getAllCommands (Array<CommandID>& commands)
{
   #if defined (EL_PRO) && EL_DOCKING
    commands.addArray ({
      
        Commands::workspaceSave,
        Commands::workspaceOpen,
        Commands::workspaceResetActive,
        Commands::workspaceSaveActive,
        Commands::workspaceClassic,
        Commands::workspaceEditing
    });
   #endif
}

void WorkspacesController::getCommandInfo (CommandID command, ApplicationCommandInfo& result)
{
   #if defined (EL_PRO) && EL_DOCKING
    switch (command)
    {
        case Commands::workspaceSave:
            result.setInfo ("Save Workspace", "Save the current workspace", Commands::Categories::UserInterface, 0);
            break;
        case Commands::workspaceOpen:
            result.setInfo ("Open Workspace", "Open a saved workspace", Commands::Categories::UserInterface, 0);
            break;
        case Commands::workspaceResetActive:
        {
            result.addDefaultKeypress ('0', ModifierKeys::altModifier | ModifierKeys::shiftModifier);
            result.addDefaultKeypress (')', ModifierKeys::altModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Reset Workspace", "Reset the active workspace to it's default state.", Commands::Categories::UserInterface, 0);
        } break;
        case Commands::workspaceSaveActive:
        {
            result.setInfo ("Save Active Workspace", "Save the current workspace to disk.", 
                Commands::Categories::UserInterface, 0);
        } break;
        case Commands::workspaceClassic:
        {
            result.setInfo ("Classic Workspace", "Open the classic workspace", 
                Commands::Categories::UserInterface, 0);
            if (content)
                result.setTicked (content->getWorkspaceName() == "Classic");
            result.addDefaultKeypress ('1', ModifierKeys::altModifier | ModifierKeys::shiftModifier);
            result.addDefaultKeypress ('!', ModifierKeys::altModifier | ModifierKeys::shiftModifier);
        } break;
        case Commands::workspaceEditing: 
        {
            result.setInfo ("Editing Workspace", "Open the editing workspace", 
                Commands::Categories::UserInterface, 0);
            if (content)
                result.setTicked (content->getWorkspaceName() == "Editing");
            result.addDefaultKeypress ('2', ModifierKeys::altModifier | ModifierKeys::shiftModifier);
            result.addDefaultKeypress ('@', ModifierKeys::altModifier | ModifierKeys::shiftModifier);
        } break;
    }
   #endif
}

bool WorkspacesController::perform (const InvocationInfo& info)
{
   #if defined (EL_PRO) && EL_DOCKING
    bool handled = true;
    switch (info.commandID)
    {
        case Commands::workspaceSave:
        {
            const auto state = content->getWorkspaceState();
            FileChooser chooser ("Save Workspace", juce::File(), "*.elw", true, false);
            if (chooser.browseForFileToSave (true))
            {
                const auto state = content->getWorkspaceState();
                state.writeToXmlFile (chooser.getResult());
            }
        } break;

        case Commands::workspaceOpen:
        {
            FileChooser chooser ("Load Workspace", juce::File(), "*.elw", true, false);
            if (chooser.browseForFileToOpen())
            {
                getAppController().postMessage (
                    new WorkspaceOpenFileMessage (chooser.getResult()));
            }
        } break;

        case Commands::workspaceResetActive:
        {
            auto state = WorkspaceState::loadByName (content->getWorkspaceName());
            if (state.isValid() && content)
                content->applyWorkspaceState (state);
        } break;

        case Commands::workspaceSaveActive:
        {
            saveCurrentWorkspace();
        } break;

        case Commands::workspaceClassic:
        {
            saveCurrentAndLoadWorkspace ("Classic");
        } break;

        case Commands::workspaceEditing:
        {
            saveCurrentWorkspace();
            auto state = WorkspaceState::loadByFileOrName ("Editing");
            if (state.isValid() && content)
                content->applyWorkspaceState (state);
        } break;

        default: 
            handled = false;
            break;
    }

    if (handled)
        findSibling<GuiController>()->refreshMainMenu();

    return handled;
   #else
    return false;
   #endif
}

void WorkspacesController::saveCurrentWorkspace()
{
    if (! content)
        return;

    auto state = content->getWorkspaceState();
    if (state.isValid())
    {
        auto name = content->getWorkspaceName();
        getWorld().getSettings().setWorkspace (name);
        name << ".elw";
        state.writeToXmlFile (DataPath::workspacesDir().getChildFile (name));
    }
}

void WorkspacesController::saveCurrentAndLoadWorkspace (const String& name)
{
    saveCurrentWorkspace();
    auto state = WorkspaceState::loadByFileOrName (name);
    if (state.isValid() && content)
        content->applyWorkspaceState (state);
}

}

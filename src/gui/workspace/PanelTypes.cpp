
#include "gui/workspace/PanelTypes.h"
#include "gui/workspace/VirtualKeyboardPanel.h"
#include "gui/workspace/GraphEditorPanel.h"
#include "gui/workspace/GraphMixerPanel.h"
#include "gui/workspace/ContentViewPanel.h"
#include "gui/workspace/PluginsPanel.h"

namespace Element {

class GenericDockPanel : public DockPanel
{
public:
    GenericDockPanel (const String& panelName)
    { 
        setName (panelName);
    }
    ~GenericDockPanel() = default;

    void showPopupMenu() override
    {
        PopupMenu menu;
        menu.addItem (1, "Close Panel");
        menu.addItem (2, "Undock Panel");
        const auto result = menu.show();

        switch (result)
        {
            case 1: {
                close();
            } break;

            case 2: {
                undock();
            } break;

            default: break;
        }
    }
};

const Identifier GenericPanelType::genericType = "GenericDockPanel";
DockPanel* GenericPanelType::createPanel (const Identifier& panelType)
{
    if (panelType == genericType)
    {
        ++lastPanelNo;
        return new GenericDockPanel (String("Generic ") + String(lastPanelNo));
    }
    
    return nullptr;
}

DockPanel* ApplicationPanelType::createPanel (const Identifier& panelId)
{
   #if defined (EL_PRO) && EL_DOCKING
    if (panelId == PanelIDs::controllers)
        return new ControllerDevicesPanel();
    if (panelId == PanelIDs::maps)
        return new ControllerMapsPanel();

    if (panelId == PanelIDs::graphMixer)
        return new GraphMixerPanel();
    if (panelId == PanelIDs::graphEditor)
        return new GraphEditorPanel();
    if (panelId == PanelIDs::graphSettings)
        return new GraphSettingsPanel();

    if (panelId == PanelIDs::keymaps)
        return new KeymapEditorPanel();

    if (panelId == PanelIDs::nodeChannelStrip)
        return new NodeChannelStripPanel();
    if (panelId == PanelIDs::nodeEditor)
        return new NodeEditorPanel();
    if (panelId == PanelIDs::nodeMidi)
        return new NodeMidiPanel();

    if (panelId == PanelIDs::virtualKeyboard)
        return new VirtualKeyboardPanel();
    
    if (panelId == PanelIDs::plugins)
        return new PluginsPanel();

   #endif
    return nullptr;
}

}

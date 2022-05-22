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
            }
            break;

            case 2: {
                undock();
            }
            break;

            default:
                break;
        }
    }
};

const Identifier GenericPanelType::genericType = "GenericDockPanel";
DockPanel* GenericPanelType::createPanel (const Identifier& panelType)
{
    if (panelType == genericType)
    {
        ++lastPanelNo;
        return new GenericDockPanel (String ("Generic ") + String (lastPanelNo));
    }

    return nullptr;
}

DockPanel* ApplicationPanelType::createPanel (const Identifier& panelId)
{
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

    if (panelId == PanelIDs::session)
        return new SessionPanel();
    if (panelId == PanelIDs::sessionSettings)
        return new SessionSettingsPanel();

    if (panelId == PanelIDs::virtualKeyboard)
        return new VirtualKeyboardPanel();

    if (panelId == PanelIDs::plugins)
        return new PluginsPanel();

    if (panelId == PanelIDs::luaConsole)
        return new LuaConsolePanel();
    if (panelId == PanelIDs::pluginManager)
        return new PluginManagerPanel();
#if 0
    if (panelId == PanelIDs::codeEditor)
        return new CodeEditorPanel();
#endif
    return nullptr;
}

} // namespace Element

/*
    PluginListWindow.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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

#ifndef ELEMENT_PLUGINLIST_WINDOW_H
#define ELEMENT_PLUGINLIST_WINDOW_H

#include "../Globals.h"
#include "Window.h"

namespace Element {
namespace Gui {

    class PluginListWindow  : public Window
    {
    public:

        PluginListWindow (Globals& world, const File& deadmansFile = File::nonexistent)
            : Window ("Plugin Manager", gui),
              pluginList (world.plugins().availablePlugins()),
              pluginManager (world.plugins())
        {
#if 0
            const File deadMansPedalFile (getAppProperties().getUserSettings()
                                          ->getFile().getSiblingFile ("RecentlyCrashedPluginsList"));

            setContentOwned (listComponent = new PluginListComponent (pluginManager.formats(), pluginList,
                                    deadmansFile, world.settings().getUserSettings()),
                             true);
#else
            setContentOwned (listComponent = new PluginListComponent (pluginManager.formats(), pluginList,
                                                                      File::nonexistent, nullptr), true);
#endif
            setResizable (true, false);
            setResizeLimits (300, 400, 800, 1500);
            setTopLeftPosition (60, 60);

            setVisible (true);
        }

        ~PluginListWindow()
        {
            clearContentComponent();
        }


        void closeButtonPressed()
        {
            closedSignal();
        }

    private:

        KnownPluginList& pluginList;
        PluginManager&   pluginManager;
        PluginListComponent* listComponent;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListWindow)
    };

}
}
#endif // ELEMENT_PLUGINLIST_WINDOW_H

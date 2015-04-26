/*
    GraphEditorPanel.cpp - This file is part of Element
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


#include "../engine/InternalFormat.h"

#include "GuiCommon.h"
#include "GraphEditorPanel.h"
#include "GuiApp.h"

namespace Element {
namespace Gui {

    typedef InternalFormat Internals;
    
    GraphEditorPanel::GraphEditorPanel (GuiApp& gui, GraphController& ctl)
        : GraphEditorBase (ctl)
    { }

    GraphEditorPanel::~GraphEditorPanel()
    { }

    void
    GraphEditorPanel::mouseDown(const MouseEvent &e)
    {
        if (! e.mods.isPopupMenu())
        {
            GraphEditorBase::mouseDown (e);
            return;
        }

        PopupMenu m;
        {
            m.addSectionHeader ("Plugins");
            PluginManager& pm (graph.plugins());
            KnownPluginList& plugs (graph.plugins().availablePlugins());

            Internals* internals = pm.format<Internals>();
            if (! internals)
                return;

            plugs.addToMenu (m, KnownPluginList::sortByManufacturer);

            m.addSectionHeader ("Internals");
            m.addItem (3, "BTSP-1");
            m.addItem (4, "Pattern");

            m.addSeparator();

            m.addItem (1, "Audio Input Device");
            m.addItem (2, "Audio Output Device");
            m.addItem (5, "MIDI Device");

            int res = m.show();

            if (res == 1)
            {

                createNewPlugin (internals->description (InternalFormat::audioInputDevice), e.x, e.y);
            }
            else if (res == 2)
            {
                createNewPlugin (internals->description (InternalFormat::audioOutputDevice), e.x, e.y);
            }
            else if (res == 3)
            {
                createNewPlugin (internals->description (InternalFormat::samplerProcessor), e.x, e.y);
            }
            else if (res == 4)
            {

            }
            else if (const PluginDescription* desc = plugs.getType (plugs.getIndexChosenByMenu (res)))
            {
                createNewPlugin (desc, e.x, e.y);
            }
        }

    }
}}

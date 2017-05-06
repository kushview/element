/*
    GraphEditorView.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

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

#include "controllers/GraphController.h"
#include "engine/InternalFormat.h"
#include "gui/GuiCommon.h"
#include "gui/GraphEditorView.h"
#include "gui/GuiApp.h"
#include "session/PluginManager.h"

namespace Element {

    typedef InternalFormat Internals;
    
    GraphEditorView::GraphEditorView (GuiApp& gui, GraphController& ctl)
            : GraphEditorBase (ctl)
    { }

    GraphEditorView::~GraphEditorView()
    { }

    void GraphEditorView::mouseDown(const MouseEvent &e)
    {
        if (! e.mods.isPopupMenu())
        {
            GraphEditorComponent::mouseDown (e);
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

            PopupMenu plugmenu;
            plugs.addToMenu (plugmenu, KnownPluginList::sortByManufacturer);
            m.addSubMenu ("Plugins", plugmenu);
            
            m.addSectionHeader ("Internals");
            m.addItem (3, "MIDI Sequence");
            //m.addItem (4, "Pattern");

            m.addSeparator();

            m.addItem (1, "Audio Input Device");
            m.addItem (2, "Audio Output Device");
            m.addItem (5, "MIDI Input");

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
                createNewPlugin (internals->description (InternalFormat::midiSequence), e.x, e.y);
            }
            else if (res == 4)
            {

            }
            else if (res == 5)
            {
                createNewPlugin (internals->description (InternalFormat::midiInputDevice), e.x, e.y);
            }
            else if (const PluginDescription* desc = plugs.getType (plugs.getIndexChosenByMenu (res)))
            {
                createNewPlugin (desc, e.x, e.y);
            }
        }

    }
}

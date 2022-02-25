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

#ifndef EL_PLUGINS_PANEL_VIEW_H
#define EL_PLUGINS_PANEL_VIEW_H

#include "gui/ContentComponent.h"

namespace Element {

class PluginManager;

class PluginsPanelView : public ContentView,
                         public ChangeListener,
                         public TextEditor::Listener,
                         public Timer
{
public:
    PluginsPanelView (PluginManager& pm);
    ~PluginsPanelView();

    void resized() override;
    void paint (Graphics&) override;

    /** Returns the text in the search box */
    String getSearchText() { return search.getText(); }

    /** @internal */
    void textEditorTextChanged (TextEditor&) override;
    void textEditorReturnKeyPressed (TextEditor&) override;
    void changeListenerCallback (ChangeBroadcaster*) override;
    void timerCallback() override;

private:
    PluginManager& plugins;
    TreeView tree;
    TextEditor search;

    void updateTreeView();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginsPanelView);
};

} // namespace Element

#endif // EL_PLUGINS_PANEL_VIEW_H

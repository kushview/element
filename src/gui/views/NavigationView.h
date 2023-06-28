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

#pragma once

#include "ElementApp.h"
#include "gui/TreeviewBase.h"

namespace element {

class NavigationList;
class NavigationTree;
class PluginManager;

class NavigationView : public Component
{
public:
    NavigationView();
    ~NavigationView();

    void paint (Graphics&);
    void resized();

private:
    std::unique_ptr<NavigationList> navList;
    std::unique_ptr<NavigationTree> navTree;
    std::unique_ptr<StretchableLayoutResizerBar> navBar;
    StretchableLayoutManager layout;

    friend class NavigationList;
    friend class NavigationTree;
    void updateLayout();
    void setRootItem (int item);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NavigationView)
};

class PluginTreeView : public TreePanelBase
{
public:
    PluginTreeView (PluginManager&);
    ~PluginTreeView();

    void rootItemChanged (int item);

private:
    int rootItem;
    PluginManager& plugins;
};

} // namespace element

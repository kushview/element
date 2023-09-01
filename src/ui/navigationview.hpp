// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "ElementApp.h"
#include "ui/treeviewbase.hpp"

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

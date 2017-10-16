#pragma once

#include "ElementApp.h"
#include "gui/TreeviewBase.h"

namespace Element {

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
    ScopedPointer<NavigationList> navList;
    ScopedPointer<NavigationTree> navTree;
    ScopedPointer<StretchableLayoutResizerBar> navBar;
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

}

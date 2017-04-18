
#include "gui/NavigationView.h"
#include "session/PluginManager.h"

#include "gui/PluginsPanelView.h"

namespace Element {
    PluginsNavigationPanel::PluginsNavigationPanel (PluginManager& pm)
    {
        addAndMakeVisible (pluginTree = new PluginTreeView());
        pluginTree->rootItemChanged (0);
        pm.availablePlugins().addChangeListener (this);
    }
    
    PluginsNavigationPanel::~PluginsNavigationPanel()
    {
    }

    void PluginsNavigationPanel::resized()
    {
        pluginTree->setBounds (getLocalBounds());
    }
    
    void PluginsNavigationPanel::paint (Graphics& g)
    {
        g.fillAll (LookAndFeel_E1::widgetBackgroundColor);
    }
    
    void PluginsNavigationPanel::changeListenerCallback (ChangeBroadcaster* src)
    {
        pluginTree->rootItemChanged (0);
    }
}

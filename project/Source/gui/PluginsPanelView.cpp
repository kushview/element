
#include "gui/NavigationView.h"
#include "session/PluginManager.h"

#include "gui/PluginsPanelView.h"

namespace Element {
    PluginsPanelView::PluginsPanelView (PluginManager& pm)
    {
        addAndMakeVisible (pluginTree = new PluginTreeView (pm));
        pluginTree->rootItemChanged (0);
        pm.availablePlugins().addChangeListener (this);
    }
    
    PluginsPanelView::~PluginsPanelView()
    {
        pluginTree = nullptr;
    }

    void PluginsPanelView::resized()
    {
        pluginTree->setBounds (getLocalBounds());
    }
    
    void PluginsPanelView::paint (Graphics& g)
    {
        g.fillAll (LookAndFeel_KV1::widgetBackgroundColor);
    }
    
    void PluginsPanelView::changeListenerCallback (ChangeBroadcaster* src)
    {
        pluginTree->rootItemChanged (0);
    }
}

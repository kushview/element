
#include "session/PluginManager.h"
#include "gui/GuiCommon.h"
#include "gui/PluginsPanelView.h"

namespace Element
{
    class PluginTreeViewItem : public TreeViewItem
    {
    public:
        PluginTreeViewItem (const PluginDescription& d) : desc (new  PluginDescription(d)) { }
        bool mightContainSubItems() override { return false; }
        const ScopedPointer<const PluginDescription> desc;

        var getDragSourceDescription() override
        {
            var result;
            result.append ("plugin");
            result.append (desc->createIdentifierString());
            return result;
        }
        
        void paintItem (Graphics& g, int width, int height) override
        {
            g.setColour (Element::LookAndFeel::textColor);
            String text = desc->name;
            g.drawText (text, 6, 0, width - 6, height, Justification::centredLeft);
        }
    };
    
    class PluginFolderTreeViewItem : public TreeViewItem
    {
    public:
        PluginFolderTreeViewItem (KnownPluginList::PluginTree& t) : tree (t) { }
        bool mightContainSubItems() override { return true; }
        KnownPluginList::PluginTree& tree;
        
        void paintItem (Graphics& g, int width, int height) override
        {
            g.setColour (Element::LookAndFeel::textColor);
            g.drawText (tree.folder, 6, 0, width - 6, height, Justification::centredLeft);
        }
        
        void itemOpennessChanged (bool isNowOpen) override
        {
            if (isNowOpen)
            {
                for (auto* folder : tree.subFolders)
                    addSubItem (new PluginFolderTreeViewItem (*folder));
                for (const auto* plugin : tree.plugins)
                    addSubItem (new PluginTreeViewItem (*plugin));
            }
            else
            {
                clearSubItems();
            }
        }
    };
    
    class PluginsPanelTreeRootItem : public TreeViewItem
    {
    public:
        PluginsPanelTreeRootItem (PluginManager& p)
            : plugins (p)
        {
            data = plugins.availablePlugins().createTree (KnownPluginList::sortByCategory);
        }
        
        bool mightContainSubItems() override { return true; }
        
        void itemOpennessChanged (bool isNowOpen) override
        {
            if (isNowOpen)
            {
                for (auto* folder : data->subFolders)
                    addSubItem (new PluginFolderTreeViewItem (*folder));
            }
            else
            {
                clearSubItems();
            }
        }
        
        PluginManager& plugins;
        ScopedPointer<KnownPluginList::PluginTree> data;
    };
    
    PluginsPanelView::PluginsPanelView (PluginManager& p)
        : plugins(p)
    {
        addAndMakeVisible (tree);
        tree.setRootItemVisible (false);
        tree.setRootItem (new PluginsPanelTreeRootItem (plugins));
        plugins.availablePlugins().addChangeListener (this);
    }
    
    PluginsPanelView::~PluginsPanelView()
    {
        plugins.availablePlugins().removeChangeListener (this);
        tree.deleteRootItem();
    }

    void PluginsPanelView::resized()
    {
        tree.setBounds (getLocalBounds().reduced (2));
    }
    
    void PluginsPanelView::paint (Graphics& g)
    {
    
    }
    
    void PluginsPanelView::changeListenerCallback (ChangeBroadcaster* src)
    {
        tree.deleteRootItem();
        tree.setRootItem (new PluginsPanelTreeRootItem (plugins));
    }
}

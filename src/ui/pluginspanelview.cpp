// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/plugins.hpp>
#include "ui/guicommon.hpp"
#include "ui/pluginspanelview.hpp"

namespace element {

class PluginTreeViewItem : public TreeViewItem
{
public:
    PluginTreeViewItem (const PluginDescription& d)
        : desc (new PluginDescription (d)) {}
    bool mightContainSubItems() override { return false; }
    const std::unique_ptr<const PluginDescription> desc;

    var getDragSourceDescription() override
    {
        var result;
        result.append ("plugin");
        result.append (desc->createIdentifierString());
        return result;
    }

    static String shortFormatName (const String& name)
    {
        if (name == "VST")
            return "vst";
        else if (name == "AudioUnit")
            return "au";
        else if (name == "VST3")
            return "vst3";
        return String();
    }
    void paintItem (Graphics& g, int width, int height) override
    {
        g.setColour (element::Colors::textColor.darker (0.22f));
        String text = desc->name;
        String extra = shortFormatName (desc->pluginFormatName);

        const int leftSide = (width * 4) / 5;
        g.drawText (text, 0, 0, leftSide, height, Justification::centredLeft);
        if (extra.isNotEmpty())
        {
            g.setColour (element::Colors::textColor.withAlpha (0.8f));
            extra = String ("(") + extra + String (")");
            g.setFont (Font (FontOptions (12.f)));
            g.drawText (extra, leftSide, 0, width - leftSide - 3, height, Justification::centredRight);
        }
    }
};

class PluginFolderTreeViewItem : public TreeViewItem
{
public:
    PluginFolderTreeViewItem (PluginsPanelView& o, KnownPluginList::PluginTree& t)
        : tree (t), panel (o)
    {
    }

    bool mightContainSubItems() override { return true; }
    KnownPluginList::PluginTree& tree;
    PluginsPanelView& panel;
    void paintItem (Graphics& g, int width, int height) override
    {
        g.setColour (element::Colors::textColor);
        g.drawText (tree.folder, 6, 0, width - 6, height, Justification::centredLeft);
    }

    void itemOpennessChanged (bool isNowOpen) override
    {
        if (isNowOpen)
        {
            const auto text = panel.getSearchText();
            for (auto* folder : tree.subFolders)
                addSubItem (new PluginFolderTreeViewItem (panel, *folder));
            for (const auto& plugin : tree.plugins)
                if (text.isEmpty() || plugin.name.containsIgnoreCase (text))
                    addSubItem (new PluginTreeViewItem (plugin));
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
    PluginsPanelTreeRootItem (PluginsPanelView& o, PluginManager& p)
        : owner (o),
          plugins (p)
    {
        data = KnownPluginList::createTree (p.getKnownPlugins().getTypes(),
                                            KnownPluginList::sortByCategory);
    }

    bool mightContainSubItems() override { return true; }

    void itemOpennessChanged (bool isNowOpen) override
    {
        if (isNowOpen)
        {
            for (auto* folder : data->subFolders)
                addSubItem (new PluginFolderTreeViewItem (owner, *folder));
        }
        else
        {
            clearSubItems();
        }
    }

    PluginsPanelView& owner;
    PluginManager& plugins;

    std::unique_ptr<KnownPluginList::PluginTree> data;
};

PluginsPanelView::PluginsPanelView (PluginManager& p)
    : plugins (p)
{
    addAndMakeVisible (search);
    search.setTextToShowWhenEmpty (TRANS ("Search..."), Colors::textColor.darker());
    search.addListener (this);

    addAndMakeVisible (tree);
    tree.setRootItemVisible (false);
    tree.setOpenCloseButtonsVisible (true);
    tree.setIndentSize (10);
    tree.setRootItem (new PluginsPanelTreeRootItem (*this, plugins));
    plugins.getKnownPlugins().addChangeListener (this);
}

PluginsPanelView::~PluginsPanelView()
{
    plugins.getKnownPlugins().removeChangeListener (this);
    tree.getRootItem()->clearSubItems();
    tree.deleteRootItem();
}

void PluginsPanelView::resized()
{
    auto r (getLocalBounds().reduced (2));
    search.setBounds (r.removeFromTop (22));
    r.removeFromTop (2);
    tree.setBounds (r);
}

void PluginsPanelView::paint (Graphics& g) {}

void PluginsPanelView::textEditorTextChanged (TextEditor&)
{
    startTimer (200);
}

void PluginsPanelView::updateTreeView()
{
    tree.deleteRootItem();
    tree.setRootItem (new PluginsPanelTreeRootItem (*this, plugins));
    auto* root = tree.getRootItem();
    for (int i = 0; i < root->getNumSubItems(); ++i)
        root->getSubItem (i)->setOpenness (TreeViewItem::Openness::opennessOpen);
}

void PluginsPanelView::timerCallback()
{
    updateTreeView();
    stopTimer();
}

void PluginsPanelView::textEditorReturnKeyPressed (TextEditor& e)
{
    stopTimer();
    updateTreeView();
}

void PluginsPanelView::changeListenerCallback (ChangeBroadcaster* src)
{
    tree.deleteRootItem();
    tree.setRootItem (new PluginsPanelTreeRootItem (*this, plugins));
}

} // namespace element

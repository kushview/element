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

#include "engine/AudioEngine.h"
#include "gui/ContentComponent.h"
#include "gui/views/NavigationView.h"
#include "gui/TreeviewBase.h"
#include "gui/ViewHelpers.h"
#include "session/pluginmanager.hpp"
#include "globals.hpp"

namespace Element {

class NavigationList : public ListBox,
                       public ListBoxModel
{
public:
    enum RootType
    {
        //        instrumentsItem = 0,
        pluginsItem = 0,
        sessionItem,
        numRootTypes
    };

    NavigationList (NavigationView* v)
        : view (v)
    {
        setModel (this);
        updateContent();
    }

    ~NavigationList()
    {
        setModel (nullptr);
    }

    static const String& getRootItemName (const int t)
    {
        jassert (t < numRootTypes);
        static const String _names[numRootTypes] = { "Plugins", "Session" };
        return _names[t];
    }

    int getNumRows() override
    {
        return numRootTypes;
    }

    void paintListBoxItem (int row, Graphics& g, int width, int height, bool selected) override
    {
        const String& name (getRootItemName (row));
        ViewHelpers::drawBasicTextRow (name, g, width, height, selected);
    }

    Component* refreshComponentForRow (int rowNumber, bool isRowSelected, Component* existingComponentToUpdate) override
    {
        return ListBoxModel::refreshComponentForRow (rowNumber, isRowSelected, existingComponentToUpdate);
    }

    void listBoxItemClicked (int row, const MouseEvent&) override
    {
        jassert (isPositiveAndBelow (row, (int) numRootTypes));
        view->setRootItem (row);
    }

    void listBoxItemDoubleClicked (int, const MouseEvent&) override {}
    void backgroundClicked (const MouseEvent&) override {}
    void selectedRowsChanged (int lastRowSelected) override {}
    void deleteKeyPressed (int) override {}
    void returnKeyPressed (int) override {}
    void listWasScrolled() override {}

    var getDragSourceDescription (const SparseSet<int>& rowsToDescribe) override
    {
        return var();
    }

    String getTooltipForRow (int row) override
    {
        switch (row)
        {
                //            case instrumentsItem:
                //                return "Built-in instruments";
                //                break;
            case pluginsItem:
                return "Available system plugins";
                break;
            case sessionItem:
                return "Current session resources";
                break;
        }
        return "Invalid Item";
    }

private:
    NavigationView* view;
};

class PluginTreeItem : public TreeItemBase
{
public:
    PluginTreeItem (const PluginDescription& d) : desc (d) {}
    ~PluginTreeItem() {}
    bool mightContainSubItems() override { return false; }
    virtual String getRenamingName() const override { return desc.name; }
    virtual String getDisplayName() const override { return desc.name; }
    virtual void setName (const String&) override {}
    virtual bool isMissing() override { return false; }
    virtual Icon getIcon() const override { return Icon(); }

    void itemDoubleClicked (const MouseEvent& ev) override {}

    var getDragSourceDescription() override
    {
        var dd;
        dd.append ("element://dnd/plugin");
        dd.append (desc.pluginFormatName);
        dd.append (desc.fileOrIdentifier);

        return dd;
    }

    const PluginDescription desc;
};

class PluginsNavigationItem : public TreeItemBase
{
public:
    PluginManager& plugins;

    PluginsNavigationItem (PluginManager& pm)
        : plugins (pm)
    {
        types = plugins.getKnownPlugins().getTypes();
    }

    ~PluginsNavigationItem() {}

    bool mightContainSubItems() override { return true; }
    String getRenamingName() const override { return "Plugins"; }
    String getDisplayName() const override { return "Plugins"; }
    void setName (const String&) override {}
    bool isMissing() override { return false; }
    Icon getIcon() const override { return Icon (getIcons().jigsaw, Colors::elemental); }
    void addSubItems() override
    {
        for (const auto& type : types)
            addSubItem (new PluginTreeItem (type));
    }

    void itemOpennessChanged (bool isOpen) override
    {
        if (isOpen)
            addSubItems();
        else
            clearSubItems();
    }

    Array<PluginDescription> types;
};

class SessionNavigationItem : public TreeItemBase
{
public:
    SessionNavigationItem() {}
    ~SessionNavigationItem() {}
    bool mightContainSubItems() override { return true; }
    String getRenamingName() const override { return "Session"; }
    String getDisplayName() const override { return "Session"; }
    virtual void setName (const String&) override {}
    virtual bool isMissing() override { return false; }
    virtual Icon getIcon() const override { return Icon (getIcons().document, Colors::elemental); }
    virtual void addSubItems() override
    {
        addSubItem (new EngineItem());
    }

    void itemOpennessChanged (bool isOpen) override
    {
        if (isOpen)
            addSubItems();
        else
            clearSubItems();
    }

private:
    class EngineItem : public TreeItemBase
    {
    public:
        EngineItem() {}
        ~EngineItem() {}

        bool mightContainSubItems() override { return true; }
        String getRenamingName() const override { return "Engine"; }
        String getDisplayName() const override { return "Engine"; }
        void setName (const String&) override {}
        bool isMissing() override { return false; }
        Icon getIcon() const override { return Icon (getIcons().document, Colors::elemental); }

        void addSubItems() override
        {
            ContentComponent* cc = getOwnerView()->findParentComponentOfClass<ContentComponent>();
            if (! cc)
                return;

            ReferenceCountedArray<Element::NodeObject> nodes;
            for (int i = 0; i < nodes.size(); ++i)
                addSubItem (new PluginInstanceItem (nodes.getUnchecked (i)));
        }

        void itemOpennessChanged (bool isOpen) override
        {
            if (isOpen)
                addSubItems();
            else
                clearSubItems();
        }
    };

    class PluginInstanceItem : public TreeItemBase
    {
    public:
        PluginInstanceItem (NodeObjectPtr n) : node (n)
        {
            jassert (node != nullptr);
            instance = node->getAudioPluginInstance();
        }

        ~PluginInstanceItem()
        {
            instance = nullptr;
            node = nullptr;
        }

        bool mightContainSubItems() override { return false; }
        String getRenamingName() const override { return (instance) ? instance->getName() : "Invalid"; }
        String getDisplayName() const override { return getRenamingName(); }
        void setName (const String&) override {}
        bool isMissing() override { return false; }
        Icon getIcon() const override { return Icon (getIcons().document, Colors::elemental); }

        void itemDoubleClicked (const MouseEvent& ev) override
        {
            if (! node || ! instance)
                return;
        }

        NodeObjectPtr node;
        AudioPluginInstance* instance;
    };
};

class NavigationTree : public TreePanelBase
{
public:
    NavigationTree (NavigationView* v)
        : TreePanelBase ("navigation"),
          view (v)
    {
        setEmptyTreeMessage ("Empty...");
    }

    void rootItemChanged (int item)
    {
        if (item == rootItem)
            return;

        switch (item)
        {
            case NavigationList::pluginsItem:
                // FIXME: setRoot (new PluginsNavigationItem ());
                break;
            case NavigationList::sessionItem:
                setRoot (new SessionNavigationItem());
                break;
            default:
                setRoot (nullptr);
                break;
        }

        rootItem = item;
    }

    ~NavigationTree() {}

private:
    NavigationView* view;
    int rootItem;
};

PluginTreeView::PluginTreeView (PluginManager& pm)
    : TreePanelBase ("plugins"),
      plugins (pm)
{
    setEmptyTreeMessage ("Empty...");
}

PluginTreeView::~PluginTreeView() {}

void PluginTreeView::rootItemChanged (int item)
{
    setRoot (new PluginsNavigationItem (plugins));
    rootItem = item;
}

NavigationView::NavigationView()
{
    addAndMakeVisible (navList = new NavigationList (this));
    addAndMakeVisible (navBar = new StretchableLayoutResizerBar (&layout, 1, true));
    addAndMakeVisible (navTree = new NavigationTree (this));
    updateLayout();
    resized();

    setRootItem (NavigationList::sessionItem);
}

NavigationView::~NavigationView()
{
    navTree = nullptr;
    navBar = nullptr;
    navList = nullptr;
}

void NavigationView::paint (Graphics& g)
{
    g.fillAll (LookAndFeel_KV1::backgroundColor);
}

void NavigationView::resized()
{
    Component* comps[] = { navList.get(), navBar.get(), navTree.get() };
    layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), false, true);
}

void NavigationView::setRootItem (int item)
{
    static bool stopRecursion = false;

    if (stopRecursion)
        return;

    stopRecursion = true;
    if (navList->getSelectedRow() != item)
        navList->selectRow (item);
    navTree->rootItemChanged (item);
    stopRecursion = false;
}

void NavigationView::updateLayout()
{
    layout.setItemLayout (0, 50.0, 200.0, 100.0);
    layout.setItemLayout (1, 3, 3, 3);
    layout.setItemLayout (2, 50.0, 200.0, 100.0);
}

} // namespace Element

/*
    SessionTreePanel.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "gui/GuiCommon.h"
#include "gui/ContextMenus.h"
#include "gui/ContentComponent.h"
#include "gui/SessionTreePanel.h"
#include "gui/ViewHelpers.h"
#include "session/Node.h"

namespace Element {

SessionGraphsListBox::SessionGraphsListBox (Session* s)
    : session (nullptr)
{
    setModel (this);
    updateContent();
}
    
SessionGraphsListBox::~SessionGraphsListBox()
{
    setModel (nullptr);
    session = nullptr;
}

int SessionGraphsListBox::getNumRows()
{
    return (session) ? session->getNumGraphs() : 0;
}
    
void SessionGraphsListBox::paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                                             bool rowIsSelected)
{
    if (! session)
        return;
    const Node node (session->getGraph (rowNumber));
    ViewHelpers::drawBasicTextRow ("  " + node.getName(), g, width, height, rowIsSelected);
}

class SessionNodeTreeItem : public TreeItemBase
{
    
public:
    SessionNodeTreeItem (const Node& n)
        : node (n)
    {
        ValueTree child (n.getValueTree());
        ValueTree parent (child.getParent());
        
        if (parent.isValid())
        {
            uniqueName = String (parent.indexOf (child));
        }
        else
        {
            uniqueName = String ((int64) node.getNodeId());
        }        
    }
    
    String getUniqueName() const override { return uniqueName; }
    
    void itemOpennessChanged (const bool isOpen) override
    {
        if (isOpen)
            refreshSubItems();
        else
            clearSubItems();
    }

    void addSubItems() override
    {
        const auto nodes (node.getNodesValueTree());
        for (int i = 0; i < nodes.getNumChildren(); ++i)
        {
            const Node c (nodes.getChild (i), false);
            if (! c.isIONode())   
                addSubItem (new SessionNodeTreeItem (Node (nodes.getChild(i), false)));
        }
    }

    void itemClicked (const MouseEvent& ev) override
    {
        if (ev.x < roundFloatToInt (1.f + getIconSize())) {
            setOpen (! isOpen());
        }

        TreeItemBase::itemClicked (ev);
    }

    virtual void itemDoubleClicked (const MouseEvent& ev) override
    {
        if (ev.x < roundFloatToInt (1.f + getIconSize()))
        {
            // icon double clicked
        }
        else if (! ev.mods.isPopupMenu())
        {
            showRenameBox();
        }
    }

    void showDocument() override
    {
        auto session = ViewHelpers::getSession (getOwnerView());
        auto* cc = ViewHelpers::findContentComponent (getOwnerView());
        auto* gui = cc->getAppController().findChild<GuiController>();

        if (node.isRootGraph())
        {
            if (node != session->getCurrentGraph())
            {
                gui->closeAllPluginWindows (true);
                auto graphs = session->getValueTree().getChildWithName (Tags::graphs);

                graphs.setProperty (Tags::active, graphs.indexOf (node.getValueTree()), 0);
                auto& app (ViewHelpers::findContentComponent(getOwnerView())->getAppController());
                app.findChild<EngineController>()->setRootNode (node);
                
            }
        }

        if (auto* cc = ViewHelpers::findContentComponent (getOwnerView()))
        {
            auto graph = (node.isGraph()) ? node : node.getParentGraph();
            cc->setCurrentNode (graph);
        }
    }

    bool mightContainSubItems() override            { return node.isGraph(); }
    String getRenamingName() const override         { return getDisplayName(); }
    String getDisplayName() const override          { return node.getName(); }

    void setName (const String& newName) override   
    {
        if (newName.isNotEmpty()) 
            node.setProperty (Tags::name, newName); 
    }
    
    
    bool isMissing() override { return false; }
    
    Icon getIcon() const override
    {
        return Icon (node.isGraph() ? getIcons().graph : getIcons().document,
                     Colors::elemental.withAlpha (0.9f));
    }

    String uniqueName;
    Node node;
    NodePopupMenu menu;
};

class SessionPluginTreeItem : public SessionNodeTreeItem
{
public:
    SessionPluginTreeItem (const Node& n) 
        : SessionNodeTreeItem (n) { }
};

class SessionRootGraphTreeItem : public SessionNodeTreeItem
{
public:
    SessionRootGraphTreeItem (const Node& n) : SessionNodeTreeItem (n) { jassert (n.isRootGraph()); }
    
    void deleteItem() override
    {
        const int index = node.getValueTree().getParent().indexOf (node.getValueTree());
        ViewHelpers::findContentComponent (getOwnerView())->getAppController()
            .findChild<EngineController>()->removeGraph (index);
    }

    void duplicateItem()
    {
        ViewHelpers::findContentComponent (getOwnerView())->getAppController()
            .findChild<EngineController>()->duplicateGraph (node);
    }

    void showSettings()
    {
        updateIndexInParent();
        ViewHelpers::invokeDirectly (getOwnerView(), Commands::showGraphConfig, false);
    }

    void editGraph()
    {
        updateIndexInParent();
        ViewHelpers::invokeDirectly (getOwnerView(), Commands::showGraphEditor, false);
    }

    int getIndexInParent() const
    {
        return node.getValueTree().getParent().indexOf (node.getValueTree());
    }

    void updateIndexInParent()
    {
        const int index = getIndexInParent();
        node.getValueTree().getParent().setProperty (Tags::active, index, 0);
    }

    void handlePopupMenuResult (int result) override
    {
        switch (result)
        {
            case 0: break;
            case 1: deleteItem(); break;
            case 2: duplicateItem(); break;
            case 3: showSettings(); break;
            case 4: editGraph(); break;
            {
                
            } break;
                
            default: break;
        }
    }

    void showPopupMenu() override
    {   
        PopupMenu menu;
        menu.addItem (1, "Remove");
        menu.addItem (2, "Duplicate");
        menu.addSeparator();
        menu.addItem (4, "Edit");
        menu.addItem (3, "Settings...");
        
        launchPopupMenu (menu);
    }
};

class SessionRootTreeItem : public TreeItemBase
{
public:
    SessionRootTreeItem (SessionTreePanel& o) : panel (o) { }

    String getUniqueName() const override { return "SESSION"; }

    void itemOpennessChanged (const bool isOpen) override
    {
        refreshSubItems();
    }

    void addSubItems() override
    {
        if (auto session = panel.getSession())
        {
            for (int i = 0; i < session->getNumGraphs(); ++i)
                addSubItem (new SessionRootGraphTreeItem (session->getGraph (i)));
        }
    }

    virtual bool mightContainSubItems() override { return true; }
    virtual String getRenamingName() const override { return getDisplayName(); }
    virtual String getDisplayName() const override { return "Session"; }
    virtual void setName (const String& newName) override { }
    virtual bool isMissing() override { return false; }
    virtual Icon getIcon() const override { return Icon (getIcons().folder, Colours::red); }

    SessionTreePanel& panel;
};

SessionTreePanel::SessionTreePanel()
    : TreePanelBase ("session")
{
    tree.setRootItemVisible (false);
    tree.setInterceptsMouseClicks (true, true);
    tree.setDefaultOpenness (true);
    setRoot (new SessionRootTreeItem (*this));
    data.addListener (this);
}

SessionTreePanel::~SessionTreePanel()
{
    data.removeListener (this);
    setRoot (nullptr);
}

void SessionTreePanel::refresh()
{
    if (rootItem)
        rootItem->refreshSubItems();
}

void SessionTreePanel::mouseDown (const MouseEvent &ev)
{

}

void SessionTreePanel::setSession (SessionPtr s)
{
    if (session == s)
    {
        refresh();
        return;
    }
    
    if (auto old = session)
    {
        // de-init for old
    }

    session = s;
    
    data = (session != nullptr) ? session->getValueTree() : ValueTree();
    refresh();
}

SessionPtr SessionTreePanel::getSession() const
{
    return session;
}



void SessionTreePanel::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
{
    
}

static bool couldBeSessionObjects (ValueTree& parent, ValueTree& child)
{
    return  parent.hasType (Tags::session) ||
            (parent.hasType (Tags::graphs) && child.hasType (Tags::node)) || 
            (parent.hasType (Tags::nodes) && child.hasType (Tags::node));
}

static void refreshSubItems (TreeItemBase* item)
{
    if (item == nullptr) return;

    if (auto* root = dynamic_cast<SessionRootTreeItem*> (item))
    {
        root->refreshSubItems();
    }
    else
    {
        item->refreshSubItems();
    }
}

void SessionTreePanel::valueTreeChildAdded (ValueTree& parent, ValueTree& child)
{
    if (couldBeSessionObjects (parent, child))
        refreshSubItems (rootItem.get());
}

void SessionTreePanel::valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int indexRemovedAt)
{
    if (couldBeSessionObjects (parent, child))
        refreshSubItems (rootItem.get());
}

void SessionTreePanel::valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex)
{
    refreshSubItems (rootItem.get());
}

void SessionTreePanel::valueTreeParentChanged (ValueTree& tree)
{

}

void SessionTreePanel::valueTreeRedirected (ValueTree& tree)
{

}

}

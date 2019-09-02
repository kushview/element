/*
    SessionTreePanel.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "controllers/SessionController.h"

#include "gui/GuiCommon.h"
#include "gui/ContextMenus.h"
#include "gui/ContentComponent.h"

#include "gui/AudioIOPanelView.h"
#include "gui/views/PluginsPanelView.h"
#include "gui/SessionTreePanel.h"
#include "gui/NavigationConcertinaPanel.h"

#include "gui/ViewHelpers.h"
#include "session/Node.h"
#include "ScopedFlag.h"

namespace Element {

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
                addSubItem (new SessionNodeTreeItem (c));
        }
    }

    void itemClicked (const MouseEvent& ev) override
    {
        if (ev.x < roundToInt (1.f + getIconSize())) {
            setOpen (! isOpen());
        }

        TreeItemBase::itemClicked (ev);
    }

    void showPluginWindow (bool showIt = true)
    {
        auto* const cc = ViewHelpers::findContentComponent (getOwnerView());
        auto* const gui = cc != nullptr ? cc->getAppController().findChild<GuiController>() : nullptr;
        if (nullptr == gui) return;

        if (showIt)
        {
            gui->presentPluginWindow (node, true);
        }
        else
        {
            if (auto* const window = gui->getPluginWindow (node))
                gui->closePluginWindow (window);
        }
    }

    void togglePluginWindow()
    {
        if (auto* const cc = ViewHelpers::findContentComponent (getOwnerView()))
        {
            if (auto* const gui = cc->getAppController().findChild<GuiController>())
            {
                if (auto* const window = gui->getPluginWindow (node))
                    gui->closePluginWindow (window);
                else
                    gui->presentPluginWindow (node, true);
            }
        }
    }
    
    virtual void itemDoubleClicked (const MouseEvent& ev) override
    {
        if (ev.x < roundToInt (1.f + getIconSize()))
        {
            togglePluginWindow();
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
        auto* const view = getOwnerView();
        auto* const tree = dynamic_cast<SessionTreePanel*> (view->getParentComponent());
        const bool hadFocus = view && view->hasKeyboardFocus (true);
        
        SharedConnectionBlock block (tree->nodeSelectedConnection, true);

        jassert(session != nullptr && cc != nullptr && gui != nullptr);
        
        auto root = node;
        while (!root.isRootGraph() && root.isValid())
            root = root.getParentGraph();

        if (root.isRootGraph() && root != session->getCurrentGraph())
        {
            ScopedFlag scopedFlag (tree->ignoreActiveRootGraphSelectionHandler, true);
            
            gui->closeAllPluginWindows (true);
            auto graphs = session->getValueTree().getChildWithName (Tags::graphs);
            graphs.setProperty (Tags::active, graphs.indexOf (root.getValueTree()), 0);
            auto& app (ViewHelpers::findContentComponent(getOwnerView())->getAppController());
            app.findChild<EngineController>()->setRootNode (root);
            if (auto* g = app.findChild<GuiController>())
                g->showPluginWindowsFor (root, true, false, false);
        }
        
        if (auto* c = ViewHelpers::findContentComponent (getOwnerView()))
        {
            auto graph = (node.isGraph()) ? node : node.getParentGraph();
            c->setCurrentNode (graph);
        }

        if (! node.isRootGraph())
            gui->selectNode (node);
        else if (node.isRootGraph() && node.hasAudioOutputNode())
            gui->selectNode (node.getNodeByFormat ("Internal", "audio.output"));
        
        gui->refreshMainMenu();
        gui->stabilizeViews();

        if (hadFocus)
            view->grabKeyboardFocus();
    }

    bool mightContainSubItems() override            { return node.isGraph(); }
    String getRenamingName() const override         { return getDisplayName(); }
    String getDisplayName() const override          { return node.getDisplayName(); }

    void setName (const String& newName) override   
    {
        node.setProperty (Tags::name, newName);
    }
    
    bool isMissing() override { return false; }
    
    Icon getIcon() const override
    {
        return Icon (node.isGraph() ? getIcons().fasThLarge : getIcons().fasRectangleLandscape,
                     Colors::elemental.withAlpha (0.9f));
    }
    
    virtual void deleteItem() override
    {
        if (! node.isRootGraph())
        {
            ViewHelpers::postMessageFor (getOwnerView(), new RemoveNodeMessage (node));
        }
    }
    
    virtual void duplicateItem()
    {
        if (! node.isRootGraph())
        {
            ViewHelpers::postMessageFor(getOwnerView(), new DuplicateNodeMessage (node));
        }
    }

    void addNewGraph()
    {
        if (! node.isGraph())
            return;

        PluginDescription desc;
        desc.fileOrIdentifier = "element.graph";
        desc.pluginFormatName = "Element";
        desc.name = "Graph";
        ViewHelpers::postMessageFor (getOwnerView(), new AddPluginMessage (node, desc));
    }
    
    void getSelectedNodes (NodeArray& nodes, bool includeRootGraphs = true)
    {
        const auto numSelected = getOwnerView()->getNumSelectedItems();
        for (int i = 0; i < numSelected; ++i)
            if (auto* const item = dynamic_cast<SessionNodeTreeItem*> (getOwnerView()->getSelectedItem (i)))
                nodes.add (item->node);
    }

    virtual void handlePopupMenuResult (int result) override
    {
        switch (result)
        {
            case 0: break;
            case 1: deleteItem(); break;
            case 2: duplicateItem(); break;
            case 5: addNewGraph(); break;

            case 10: {
                NodeArray selected; getSelectedNodes (selected);
                NodeArray nodes, rootGraphs;
                for (const auto& node : selected)
                {
                    if (node.isRootGraph())
                        rootGraphs.add (node);
                    else
                        nodes.add (node);
                }
                selected.clearQuick();
                ViewHelpers::postMessageFor (getOwnerView(), new RemoveNodeMessage (nodes));
            } break;

            default: break;
        }
    }
    
    void showMultiSelectionPopupMenu() override
    {
        PopupMenu menu;
        if (node.isGraph())
        {
            menu.addItem (5, "Add Nested Graph");
            menu.addSeparator();
        }
        
        if (! node.isRootGraph())
            menu.addItem (10, "Delete Selected");
        
        launchPopupMenu (menu);
    }

    virtual void showPopupMenu() override
    {
        PopupMenu menu;
        if (node.isGraph())
        {
            menu.addItem (5, "Add Nested Graph");
            menu.addSeparator();
        }
        
        menu.addItem (2, "Duplicate");
        menu.addSeparator();
        menu.addItem (1, "Delete");
        
        launchPopupMenu (menu);
    }
    
    bool isInterestedInDragSource (const DragAndDropTarget::SourceDetails& details) override
    {
        const auto& desc (details.description);
        if (! node.isGraph())
            return false;
        
        return desc.toString() == "ccNavConcertinaPanel"
            || (desc.isArray() && desc.size() >= 2 && desc[0] == "plugin");
    } 

    void itemDropped (const DragAndDropTarget::SourceDetails& details, int index) override
    {
        ignoreUnused (index);

        auto* world = ViewHelpers::getGlobals (getOwnerView());
        auto session = world->getSession();
        const auto& desc (details.description);

        const auto graph = node.isGraph() ? node : node.getParentGraph();

        if (desc.toString() == "ccNavConcertinaPanel")
        {
            auto* nav = ViewHelpers::getNavigationConcertinaPanel (getOwnerView());
            auto* panel = (nav) ? nav->findPanel<DataPathTreeComponent>() : 0;
            const auto file ((panel) ? panel->getSelectedFile() : File());
            if (file.hasFileExtension ("elg"))
            {
                const Node newGraph (Node::parse (file));
                ViewHelpers::postMessageFor (getOwnerView(),
                    new AddNodeMessage (newGraph, graph));
            }
        }
        else if (desc.isArray() && desc[0] == "plugin")
        {
            if (auto* p = world->getPluginManager().getKnownPlugins().getTypeForIdentifierString(desc[1].toString()))
            {
                ViewHelpers::postMessageFor (getOwnerView(),
                    new AddPluginMessage (graph, *p));
            }
        }
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

    void duplicateItem() override
    {
        ViewHelpers::findContentComponent (getOwnerView())->getAppController()
            .findChild<EngineController>()->duplicateGraph (node);
    }

    void paintContent (Graphics& g, const Rectangle<int>& area) override
    {
        TreeItemBase::paintContent (g, area);
        if (! node.isRootGraph())
            return;
       
       #ifndef EL_FREE
        // Paint the program number if it is enabled
        const auto h = area.getHeight(); const auto w = h;
        const int p = (int) node.getProperty (Tags::midiProgram, -1);
        if (p >= 0)
            g.drawText (String (1 + p), area.getWidth() - h, 0, w, h, Justification::centred);
       #endif
    }

    void showSettings()
    {
        updateIndexInParent();
        ViewHelpers::invokeDirectly (getOwnerView(), Commands::showGraphConfig, false);
    }

    void addGraph()
    {
        PluginDescription desc;
        desc.fileOrIdentifier = "element.graph";
        desc.pluginFormatName = "Element";
        desc.name = "Graph";
        ViewHelpers::postMessageFor (getOwnerView(), new AddPluginMessage (node, desc));
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
            case 5: addGraph(); break;
            {
                
            } break;
                
            default: break;
        }
    }

    void showPopupMenu() override
    {   
        PopupMenu menu;
        menu.addItem (5, "Add Nested Graph");
        menu.addItem (4, "Edit Graph...");
        menu.addItem (3, "View Settings...");
        menu.addSeparator();

        menu.addItem (2, "Duplicate");
        menu.addSeparator();
        menu.addItem (1, "Delete");
        
 
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

    bool isInterestedInDragSource (const DragAndDropTarget::SourceDetails& details) override
    {
        const auto& desc (details.description);
        return desc.toString() == "ccNavConcertinaPanel";
        // ||    (desc.isArray() && desc.size() >= 2 && desc[0] == "plugin");
    } 

    void itemDropped (const DragAndDropTarget::SourceDetails& details, int index) override
    {
        // TODO: need to not directly bind index of graph in model from the actual
        // index used in the engine.  After this, it will be less complicated to
        // insert graphs anywhere from a visual standpoint.
        ignoreUnused (index);

        auto* world = ViewHelpers::getGlobals (getOwnerView());
        auto session = world->getSession();
        auto& app (ViewHelpers::findContentComponent (getOwnerView())->getAppController());
        const auto& desc (details.description);

        if (desc.toString() == "ccNavConcertinaPanel")
        {
            auto* nav = ViewHelpers::getNavigationConcertinaPanel (getOwnerView());
            auto* panel = (nav) ? nav->findPanel<DataPathTreeComponent>() : 0;
            const auto file ((panel) ? panel->getSelectedFile() : File());
            if (file.hasFileExtension ("elg"))
            {
                if (auto* sess = app.findChild<SessionController>())
                    sess->importGraph (file);
            }
        }
    }

    #if 0
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    bool shouldDrawDragImageWhenOver() override { return true; }
    void itemDragExit (const SourceDetails& details) override;
    virtual void itemDragEnter (const SourceDetails& details) override;
    #endif

    SessionTreePanel& panel;
};

SessionTreePanel::SessionTreePanel()
    : TreePanelBase ("session")
{
    tree.setRootItemVisible (false);
    tree.setInterceptsMouseClicks (true, true);
    tree.setDefaultOpenness (true);
    tree.setMultiSelectEnabled (true);
    setRoot (new SessionRootTreeItem (*this));
    data.addListener (this);
}

SessionTreePanel::~SessionTreePanel()
{
    nodeSelectedConnection.disconnect();
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
    session = s;
    data.removeListener (this);
    data = (session != nullptr) ? session->getValueTree() : ValueTree();
    data.addListener (this);

    if (auto* const gui = ViewHelpers::getGuiController (this))
    {
        if (! nodeSelectedConnection.connected())
            nodeSelectedConnection = gui->nodeSelected.connect (
                std::bind (&SessionTreePanel::onNodeSelected, this));
    }

    refresh();
    selectActiveRootGraph();
}

SessionPtr SessionTreePanel::getSession() const
{
    return session;
}

static TreeViewItem* findItemForNodeRecursive (TreeViewItem* item, const Node& node)
{
    if (auto* const sitem = dynamic_cast<SessionNodeTreeItem*> (item))
        if (sitem->node == node)
            return sitem;
    for (int i = 0; i < item->getNumSubItems(); ++i)
    {
        if (auto* const found = findItemForNodeRecursive (item->getSubItem (i), node))
            return found;
    }

    return nullptr;
}

TreeViewItem* SessionTreePanel::findItemForNode (const Node& node) const
{
    if (rootItem != nullptr)
        return findItemForNodeRecursive (rootItem.get(), node);
    return nullptr;
}

void SessionTreePanel::onNodeSelected()
{
    if (auto* const gui = ViewHelpers::getGuiController (this))
        if (auto* const item = findItemForNode (gui->getSelectedNode()))
            item->setSelected (true, true, dontSendNotification);
}

void SessionTreePanel::selectActiveRootGraph()
{
    if (ignoreActiveRootGraphSelectionHandler || ! session || nullptr == rootItem)
        return;
    const auto activeGraph = session->getActiveGraph();
    for (int i = 0; i < rootItem->getNumSubItems(); ++i)
    {
        if (auto* const item = dynamic_cast<SessionNodeTreeItem*> (rootItem->getSubItem (i)))
        {
            if (activeGraph == item->node)
            {
                if (! item->isSelected())
                {
                   #if EL_DOCKING
                    item->setSelected (true, true, dontSendNotification);
                   #else
                    item->setSelected (true, true, sendNotificationAsync);
                   #endif
                    item->repaintItem();
                }
                break;
            }
        }
    }
}

void SessionTreePanel::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
{
    if (tree.hasType (Tags::graphs) && property == Tags::active)
    {
        selectActiveRootGraph();
    }
    else if (tree.hasType (Tags::node))
    {
        const Node graph (tree, false);
        if (property == Tags::name || (graph.isRootGraph() && property == Tags::midiProgram))
            repaint();
    }
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

bool SessionTreePanel::keyPressed (const KeyPress& k)
{
    if ((k.getKeyCode() == 'A' || k.getKeyCode() == 'a') && k.getModifiers().isCommandDown())
    {
        rootItem->getSubItem(0)->setSelected (true, true, dontSendNotification);
        return true;
    }
    else if (k.getKeyCode() == KeyPress::rightKey && k.getModifiers().isAltDown())
    {
        if (auto* const item = dynamic_cast<SessionNodeTreeItem*> (tree.getSelectedItem (0)))
        {
            item->showPluginWindow (true);
            return true;
        }
    }

    return TreePanelBase::keyPressed (k);
}

}

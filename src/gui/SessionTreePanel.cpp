/*
    This file is part of Element.
    Copyright (C) 2019-2021  Kushview, LLC.  All rights reserved.

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

#include <element/script.hpp>
#include <element/graph.hpp>
#include "gui/datapathbrowser.hpp"

#include "gui/views/GraphEditorView.h"
#include "gui/views/scriptview.hpp"
#include "services/sessionservice.hpp"
#include "engine/nodes/NodeTypes.h"

#include "gui/GuiCommon.h"
#include "gui/ContextMenus.h"
#include <element/ui/content.hpp>

#include "gui/AudioIOPanelView.h"
#include "gui/views/PluginsPanelView.h"
#include "gui/views/ScriptEditorView.h"
#include "gui/SessionTreePanel.h"
#include <element/ui/navigation.hpp>

#include "gui/ViewHelpers.h"
#include <element/node.hpp>
#include "scopedflag.hpp"

namespace element {

//=============================================================================
class SessionBaseTreeItem : public TreeItemBase
{
public:
    SessionBaseTreeItem() = default;
    virtual ~SessionBaseTreeItem() = default;

    //=========================================================================
    Content* content() const noexcept
    {
        if (auto* const tv = getOwnerView())
            return ViewHelpers::findContentComponent (tv);
        return nullptr;
    }

    SessionTreePanel* getSessionTreePanel()
    {
        if (auto view = getOwnerView())
            return dynamic_cast<SessionTreePanel*> (view->findParentComponentOfClass<SessionTreePanel>());
        return nullptr;
    }

    void refreshPanel()
    {
        if (auto panel = getSessionTreePanel())
            panel->refresh();
    }

    //=========================================================================
    virtual bool mightContainSubItems() override { return false; }

    /** @internal */
    String getUniqueName() const override { return uniqueName; }

    /** @internal */
    void itemOpennessChanged (const bool isOpen) override
    {
        if (isOpen)
            refreshSubItems();
        else
            clearSubItems();
    }

    /** @internal */
    void itemClicked (const MouseEvent& ev) override
    {
        if (ev.x < roundToInt (1.f + getIconSize()))
        {
            setOpen (! isOpen());
        }

        TreeItemBase::itemClicked (ev);
    }

protected:
    void setUniqueName (const String& n)
    {
        uniqueName = n;
    }

private:
    String uniqueName;
};

//=============================================================================
class SessionNodeTreeItem : public SessionBaseTreeItem
{
public:
    SessionNodeTreeItem (const Node& n)
        : node (n)
    {
        ValueTree child (n.data());
        ValueTree parent (child.getParent());

        if (parent.isValid())
        {
            setUniqueName (String (parent.indexOf (child)));
        }
        else
        {
            setUniqueName (String ((int64) node.getNodeId()));
        }
    }

    //=========================================================================
    Node getNode() const { return node; }

    /** Returns true if the given node refers to the viewed node */
    bool refersTo (const Node& o) const { return o.data() == node.data(); }

    void showPluginWindow (bool showIt = true)
    {
        auto* const cc = ViewHelpers::findContentComponent (getOwnerView());
        auto* const gui = cc != nullptr ? cc->services().find<GuiService>() : nullptr;
        if (nullptr == gui)
            return;

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
            if (auto* const gui = cc->services().find<GuiService>())
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
        if (session == nullptr || cc == nullptr)
            return;

        auto* gui = cc->services().find<GuiService>();
        auto* const view = getOwnerView();
        auto* const tree = getSessionTreePanel();
        if (view == nullptr || tree == nullptr)
            return;

        const bool hadFocus = view && view->hasKeyboardFocus (true);

        SharedConnectionBlock block (tree->nodeSelectedConnection, true);

        jassert (session != nullptr && cc != nullptr && gui != nullptr);

        auto root = node;
        while (! root.isRootGraph() && root.isValid())
            root = root.getParentGraph();

        if (root.isRootGraph() && root != session->getCurrentGraph())
        {
            ScopedFlag scopedFlag (tree->ignoreActiveRootGraphSelectionHandler, true);

            gui->closeAllPluginWindows (true);
            auto graphs = session->data().getChildWithName (tags::graphs);
            graphs.setProperty (tags::active, graphs.indexOf (root.data()), 0);
            auto& app (ViewHelpers::findContentComponent (getOwnerView())->services());
            app.find<EngineService>()->setRootNode (root);
            gui->showPluginWindowsFor (root, true, false, false);
        }

        if (auto* c = ViewHelpers::findContentComponent (getOwnerView()))
        {
            auto graph = (node.isGraph()) ? node : node.getParentGraph();
            c->presentView (EL_VIEW_GRAPH_EDITOR);
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

    String getRenamingName() const override { return getDisplayName(); }
    String getDisplayName() const override { return node.getDisplayName(); }

    void setName (const String& newName) override
    {
        node.setProperty (tags::name, newName);
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
            ViewHelpers::postMessageFor (getOwnerView(), new DuplicateNodeMessage (node));
        }
    }

    void addNewGraph()
    {
        if (! node.isGraph())
            return;

        PluginDescription desc;
        desc.fileOrIdentifier = EL_NODE_ID_GRAPH;
        desc.pluginFormatName = EL_NODE_FORMAT_NAME;
        desc.name = types::Graph.toString();
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
            case 0:
                break;
            case 1:
                deleteItem();
                break;
            case 2:
                duplicateItem();
                break;
            case 5:
                addNewGraph();
                break;

            case 10: {
                NodeArray selected;
                getSelectedNodes (selected);
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
            }
            break;

            default:
                break;
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
        auto session = world->session();
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
            if (auto p = world->plugins().getKnownPlugins().getTypeForIdentifierString (desc[1].toString()))
            {
                ViewHelpers::postMessageFor (getOwnerView(),
                                             new AddPluginMessage (graph, *p));
            }
        }
    }

    Node node;
    NodePopupMenu menu;
};

//=============================================================================
class SessionScriptNodeTreeItem : public SessionNodeTreeItem
{
public:
    SessionScriptNodeTreeItem (const Node& n)
        : SessionNodeTreeItem (n)
    {
        jassert (node.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_SCRIPT));
    }

    bool mightContainSubItems() override { return true; }

    void addSubItems() override
    {
        addSubItem (new TreeItem (getNode(), false));
        addSubItem (new TreeItem (getNode(), true));
    }

private:
    class TreeItem : public SessionNodeTreeItem
    {
    public:
        Node node;
        const bool forUI;

        TreeItem() = delete;
        TreeItem (const Node& n, bool isUI)
            : SessionNodeTreeItem (n),
              node (n),
              forUI (isUI) {}
        ~TreeItem() = default;

        SessionScriptNodeTreeItem* getParent() const noexcept
        {
            return dynamic_cast<SessionScriptNodeTreeItem*> (getParentItem());
        }

        void showDocument() override
        {
            if (auto* cc = content())
            {
                cc->presentView (std::unique_ptr<View> (new ScriptNodeScriptEditorView (node, forUI)));
            }
        }

        String getDisplayName() const override
        {
            return String (forUI ? "UI" : "DSP");
        }

        String getRenamingName() const override { return getDisplayName(); }
    };
};

//===
class GraphScriptTreeItem : public SessionBaseTreeItem
{
public:
    GraphScriptTreeItem (const Script& s)
        : SessionBaseTreeItem(),
          script (s)
    {
    }

    bool mightContainSubItems() override { return false; }
    void addSubItems() override {}

    String getRenamingName() const override { return getDisplayName(); }
    String getDisplayName() const override { return script.name(); }

    void setName (const String& newName) override
    {
        script.setName (newName);
    }

    void showDocument() override
    {
        if (auto* cc = content())
            cc->presentView (std::unique_ptr<View> (new ScriptEditorView (script)));
    }

    bool isMissing() override { return false; }

    Icon getIcon() const override
    {
        return Icon (getIcons().fasCircle, Colors::elemental.withAlpha (0.9f));
    }

    void deleteItem() override
    {
        Script::removeFromParent (script);
        script = {};
        refreshPanel();
    }

    void handlePopupMenuResult (int res) override
    {
        if (res == 1)
        {
            deleteItem();
        }
    }

    void showPopupMenu() override
    {
        PopupMenu menu;
        menu.addItem (1, "Delete");
        launchPopupMenu (menu);
    }

private:
    Script script;
};

//=============================================================================
class SessionGraphTreeItem : public SessionNodeTreeItem
{
public:
    SessionGraphTreeItem (const Node& n)
        : SessionNodeTreeItem (n)
    {
        jassert (n.isGraph());
    }

    //=========================================================================
    bool mightContainSubItems() override { return true; }

    void addSubItems() override
    {
        const auto n = getNode();
        const auto nodes (n.getNodesValueTree());
        for (int i = 0; i < nodes.getNumChildren(); ++i)
        {
            const Node c (nodes.getChild (i), false);
            if (c.isIONode())
                continue;

            if (c.isA (EL_NODE_FORMAT_NAME, EL_NODE_ID_SCRIPT))
                addSubItem (new SessionScriptNodeTreeItem (c));
            else if (c.isGraph())
                addSubItem (new SessionGraphTreeItem (c));
            else
                addSubItem (new SessionNodeTreeItem (c));
        }
    }
};

//=============================================================================
class SessionRootGraphTreeItem : public SessionGraphTreeItem
{
public:
    SessionRootGraphTreeItem (const Node& n)
        : SessionGraphTreeItem (n)
    {
        jassert (n.isRootGraph());
    }

    void deleteItem() override
    {
        const int index = node.data().getParent().indexOf (node.data());
        ViewHelpers::findContentComponent (getOwnerView())->services().find<EngineService>()->removeGraph (index);
    }

    void showDocument() override
    {
        Graph graph (getNode());
        Script script = graph.findViewScript();
        if (script.valid())
        {
            if (auto* cc = content())
            {
                auto view = new ScriptView (cc->context(), script);
                view->setNode (node);
                cc->presentView (std::unique_ptr<View> (view));
            }
        }
        else
        {
            if (auto* cc = content())
            {
                auto s = cc->session();
                for (int i = 0; i < s->getNumGraphs(); ++i)
                {
                    if (s->getGraph (i) == graph)
                    {
                        cc->session()->setActiveGraph (i);
                        cc->presentView (EL_VIEW_GRAPH_EDITOR);
                    }
                }
            }
        }
    }

    void addSubItems() override
    {
        SessionGraphTreeItem::addSubItems();
        for (int i = 0; i < getNode().getScriptsValueTree().getNumChildren(); ++i)
        {
            Script script (getNode().getScriptsValueTree().getChild (i));
            addSubItem (new GraphScriptTreeItem (script));
        }
    }

    void duplicateItem() override
    {
        ViewHelpers::findContentComponent (getOwnerView())->services().find<EngineService>()->duplicateGraph (node);
    }

    void paintContent (Graphics& g, const Rectangle<int>& area) override
    {
        TreeItemBase::paintContent (g, area);
        if (! node.isRootGraph())
            return;

        // Paint the program number if it is enabled
        const auto h = area.getHeight();
        const int p = (int) node.getProperty (tags::midiProgram, -1);
        if (p >= 0)
        {
            const auto txt = String (1 + p);
            const auto txtW = g.getCurrentFont().getStringWidth (txt) + 2;
            g.drawText (txt, area.getWidth() - txtW, 0, txtW, h, Justification::centredRight);
        }
    }

    void showSettings()
    {
        updateIndexInParent();
        ViewHelpers::invokeDirectly (getOwnerView(), Commands::showGraphConfig, false);
    }

    void addGraph()
    {
        PluginDescription desc;
        desc.fileOrIdentifier = EL_NODE_ID_GRAPH;
        desc.pluginFormatName = EL_NODE_FORMAT_NAME;
        desc.name = types::Graph.toString();
        ViewHelpers::postMessageFor (getOwnerView(), new AddPluginMessage (node, desc));
    }

    void editGraph()
    {
        updateIndexInParent();
        ViewHelpers::invokeDirectly (getOwnerView(), Commands::showGraphEditor, false);
    }

    int getIndexInParent() const
    {
        return node.data().getParent().indexOf (node.data());
    }

    void updateIndexInParent()
    {
        const int index = getIndexInParent();
        node.data().getParent().setProperty (tags::active, index, 0);
    }

    void handlePopupMenuResult (int result) override
    {
        switch (result)
        {
            case 0:
                break;
            case 1:
                deleteItem();
                break;
            case 2:
                duplicateItem();
                break;
            case 3:
                showSettings();
                break;
            case 4:
                editGraph();
                break;
            case 5:
                addGraph();
                break;
            case 6:
            case 7:
                addScript (result);
            default:
                break;
        }
    }

    void addScript (int res)
    {
        if (res == 6)
            getNode().addScript (Script::make ("Anonymous", types::Anonymous));
        else if (res == 7)
            getNode().addScript (Script::make ("View", types::View));
        refreshPanel();
    }

    void showPopupMenu() override
    {
        bool hasView = Graph (getNode()).hasViewScript();

        PopupMenu menu;
        menu.addItem (5, "Add Nested Graph");
        PopupMenu scripts;
        scripts.addItem (6, "Anonymous");
        scripts.addItem (7, "View", ! hasView, hasView);

        menu.addSubMenu ("Add Script", scripts, true);
        menu.addSeparator();
        menu.addItem (4, "Edit Graph...");
        menu.addItem (3, "View Settings...");
        menu.addSeparator();

        menu.addItem (2, "Duplicate");
        menu.addSeparator();
        menu.addItem (1, "Delete");

        launchPopupMenu (menu);
    }
};

//=============================================================================
class SessionRootTreeItem : public TreeItemBase
{
public:
    SessionRootTreeItem (SessionTreePanel& o) : panel (o) {}

    String getUniqueName() const override { return "SESSION"; }

    void itemOpennessChanged (const bool isOpen) override
    {
        refreshSubItems();
    }

    void addSubItems() override
    {
        if (auto session = panel.session())
        {
            for (int i = 0; i < session->getNumGraphs(); ++i)
                addSubItem (new SessionRootGraphTreeItem (session->getGraph (i)));
        }
    }

    virtual bool mightContainSubItems() override { return true; }
    virtual String getRenamingName() const override { return getDisplayName(); }
    virtual String getDisplayName() const override { return "Session"; }
    virtual void setName (const String& newName) override {}
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
        auto session = world->session();
        auto& app (ViewHelpers::findContentComponent (getOwnerView())->services());
        const auto& desc (details.description);

        if (desc.toString() == "ccNavConcertinaPanel")
        {
            auto* nav = ViewHelpers::getNavigationConcertinaPanel (getOwnerView());
            auto* panel = (nav) ? nav->findPanel<DataPathTreeComponent>() : 0;
            const auto file ((panel) ? panel->getSelectedFile() : File());
            if (file.hasFileExtension ("elg"))
            {
                if (auto* sess = app.find<SessionService>())
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

//======
class SessionTreePanel::Panel : public TreePanelBase
{
public:
    Panel() {};
    ~Panel() {};
};

//=============================================================================
SessionTreePanel::SessionTreePanel()
    : Component ("Session Tree Pannel")
{
    setComponentID ("el.SessionTreePanel");
    panel = std::make_unique<Panel>();
    addAndMakeVisible (panel.get());
    auto& tree = panel->tree;
    tree.setRootItemVisible (false);
    tree.setInterceptsMouseClicks (true, true);
    tree.setDefaultOpenness (true);
    tree.setMultiSelectEnabled (true);
    panel->setRoot (new SessionRootTreeItem (*this));
    data.addListener (this);
}

SessionTreePanel::~SessionTreePanel()
{
    nodeSelectedConnection.disconnect();
    data.removeListener (this);
    panel->setRoot (nullptr);
    panel.reset();
}

void SessionTreePanel::refresh()
{
    if (auto* const gui = ViewHelpers::getGuiController (this))
    {
        if (! nodeSelectedConnection.connected())
            nodeSelectedConnection = gui->nodeSelected.connect (
                std::bind (&SessionTreePanel::onNodeSelected, this));
    }

    if (panel->rootItem)
        panel->rootItem->refreshSubItems();
}

void SessionTreePanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void SessionTreePanel::resized()
{
    panel->setBounds (getLocalBounds());
}

void SessionTreePanel::mouseDown (const MouseEvent& ev) {}

void SessionTreePanel::setSession (SessionPtr s)
{
    _session = s;

    if (! showingNode())
    {
        data.removeListener (this);
        data = (_session != nullptr) ? _session->data() : ValueTree();
        data.addListener (this);
    }

    refresh();
}

void SessionTreePanel::showNode (const Node& newNode)
{
    if (newNode == node)
        return;

    data.removeListener (this);
    node = newNode;

    panel->setRoot (nullptr);
    if (node.isRootGraph())
    {
        panel->setRoot (new SessionRootGraphTreeItem (node));
        data = node.data();
        refresh();
    }
    else if (node.isGraph())
    {
        panel->setRoot (new SessionGraphTreeItem (node));
        data = node.data();
        refresh();
    }
    else
    {
        auto os = _session;
        _session = nullptr;
        setSession (os);
    }

    data.addListener (this);
}

bool SessionTreePanel::showingNode() const noexcept { return node.isValid(); }

SessionPtr SessionTreePanel::session() const
{
    return _session;
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
    if (panel->rootItem != nullptr)
        return findItemForNodeRecursive (panel->rootItem.get(), node);
    return nullptr;
}

void SessionTreePanel::onNodeSelected()
{
    if (auto* const gui = ViewHelpers::getGuiController (this))
    {
        if (showingNode() && gui->getSelectedNode() == node)
            return;
        if (auto* const item = findItemForNode (gui->getSelectedNode()))
            item->setSelected (true, true, dontSendNotification);
    }
}

void SessionTreePanel::selectActiveRootGraph()
{
    if (ignoreActiveRootGraphSelectionHandler || ! _session || nullptr == panel->rootItem)
        return;
    const auto activeGraph = _session->getActiveGraph();
    for (int i = 0; i < panel->rootItem->getNumSubItems(); ++i)
    {
        if (auto* const item = dynamic_cast<SessionNodeTreeItem*> (panel->rootItem->getSubItem (i)))
        {
            if (activeGraph == item->node)
            {
                if (! item->isSelected())
                {
                    item->setSelected (true, true, dontSendNotification);
                    item->repaintItem();
                }
                break;
            }
        }
    }
}

void SessionTreePanel::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
{
    if (tree.hasType (tags::graphs) && property == tags::active)
    {
        selectActiveRootGraph();
    }
    else if (tree.hasType (types::Node))
    {
        const Node graph (tree, false);
        if (property == tags::name || (graph.isRootGraph() && property == tags::midiProgram))
            repaint();
    }
}

static bool couldBeSessionObjects (ValueTree& parent, ValueTree& child)
{
    // clang-format off
    return parent.hasType (types::Session) || 
        (parent.hasType (tags::graphs) && child.hasType (types::Node)) || 
        (parent.hasType (tags::nodes) && child.hasType (types::Node));
    // clang-format on
}

static void refreshSubItems (TreeItemBase* item)
{
    if (item == nullptr)
        return;

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
        refreshSubItems (panel->rootItem.get());
}

void SessionTreePanel::valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int indexRemovedAt)
{
    if (couldBeSessionObjects (parent, child))
        refreshSubItems (panel->rootItem.get());
}

void SessionTreePanel::valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex)
{
    refreshSubItems (panel->rootItem.get());
}

void SessionTreePanel::valueTreeParentChanged (ValueTree& tree)
{
}

void SessionTreePanel::valueTreeRedirected (ValueTree& tree)
{
}

bool SessionTreePanel::keyPressed (const KeyPress& k)
{
    auto rootItem = panel->rootItem.get();
    auto& tree = panel->tree;

    if ((k.getKeyCode() == 'A' || k.getKeyCode() == 'a') && k.getModifiers().isCommandDown())
    {
        rootItem->getSubItem (0)->setSelected (true, true, dontSendNotification);
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

    return panel->keyPressed (k);
}

} // namespace element

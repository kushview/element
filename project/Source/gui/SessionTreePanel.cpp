/*
    SessionTreePanel.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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

#include "GuiCommon.h"
#include "AssetTreeView.h"
#include "ContentComponent.h"
#include "NewFileWizard.h"
#include "SessionTreePanel.h"

namespace Element {
namespace Gui {


class PatternTreeItem :  public AssetTreeViewItem
{
public:

    PatternTreeItem (const AssetTree::Item& i)
        : AssetTreeViewItem (i)
    { }

    bool mightContainSubItems() { return false; }

    String getDisplayName() const { return item.getName(); }

    Icon getIcon() const { return Icon (getIcons().document, Colours::orange); }

    bool isMissing() { return ! item.getFile().existsAsFile(); }

    void showPopupMenu()
    {
        PopupMenu menu;
        menu.addItem (1, "Rename Pattern");
        menu.addItem (2, "Remove from Session");
        launchPopupMenu (menu);
    }

    var getDragSourceDescription()
    {
        String desc (item.getFile().getFullPathName());
        return desc;
    }

    void handlePopupMenuResult (int res)
    {
        switch (res)
        {
            case 1: showRenameBox(); break;
            case 2: deleteItem(); break;
            case 3: break;
        }
    }

    void deleteItem()
    {
        item.removeFromTree();
    }

    void deleteAllSelectedItems() {

    }

    void showDocument()
    { }

    void showMultiSelectionPopupMenu()
    {
    }

};

class SessionGroupItem : public GroupTreeViewItem
{
public:

    SessionGroupItem (Session& sess, const AssetItem& i)
        : GroupTreeViewItem (i), session (sess)
    { }

    AssetTreeViewItem*
    createAssetSubItem (const AssetItem &child)
    {
        if (child.isFile()) {
            return new PatternTreeItem (child);
        } else {
            return new SessionGroupItem (session, child);
        }

        return GroupTreeViewItem::createAssetSubItem (child);
    }

    Session& session;
};


class SessionRootTreeItem :  public SessionGroupItem
{
public:

    SessionRootTreeItem (GuiApp& g, SessionTreePanel& p)
        : SessionGroupItem (g.globals().session(),
                            g.globals().session().assets().root()),
          gui(g),
          session (g.globals().session()),
          panel (p)
    { }

    bool mightContainSubItems() { return true; }

    void setName (const String& newName) { session.setName (newName); }

    String getRenamingName()    const    { return getDisplayName(); }
    String getDisplayName()     const    { return session.name(); }

    bool isMissing()
    {
        return session.assets().getFile().getParentDirectory().existsAsFile();
    }

    Icon getIcon() const { return Icon (getIcons().folder, Colours::red); }

private:

    GuiApp& gui;
    Session& session;
    SessionTreePanel& panel;

};



SessionTreePanel::SessionTreePanel (GuiApp& g)
    : TreePanelBase ("session"),
      gui (g)
{
    setRoot (new SessionRootTreeItem (g, *this));
    tree.setInterceptsMouseClicks (false, true);
}

SessionTreePanel::~SessionTreePanel()
{
    setRoot (nullptr);
}

void SessionTreePanel::mouseDown (const MouseEvent &ev)
{
    if (ev.mods.isPopupMenu())
    {
        PopupMenu menu;
        NewFileWizard().addWizardsToMenu (menu);

#if JUCE_DEBUG
        menu.addSectionHeader ("Debugging");
        menu.addItem (2, "Dump XML to Console");
        menu.addItem (3, "Reset Root Item");
#endif

        const int res = menu.show();

        SessionRef s (session().makeRef());
        AssetTree& assets (s->assets());

        switch (res)
        {
            case 1:
            {
                NewFileWizard().runWizardFromMenu (res, assets.root());
                break;
            }
#if JUCE_DEBUG
            case 2:
                s->testPrintXml();
                break;
            case 3:
                setRoot (new SessionRootTreeItem (gui, *this));
                break;

#endif
            default:
                NewFileWizard().runWizardFromMenu (res, assets.root());
                break;
        }

    }


}

Session&
SessionTreePanel::session()
{
    return gui.globals().session();
}

}}

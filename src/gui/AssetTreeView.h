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

#ifndef ELEMENT_TREEVIEW_TYPES_H
#define ELEMENT_TREEVIEW_TYPES_H

#include "gui/SessionTreePanel.h"
#include "session/AssetTree.h"

namespace Element {

class AssetTreeViewItem : public TreeItemBase,
                          public ValueTree::Listener
{
public:
    AssetTreeViewItem (const AssetTree::Item& item);
    ~AssetTreeViewItem();

    virtual bool mightContainSubItems() override;
    virtual String getRenamingName() const override;
    virtual String getDisplayName() const override;
    virtual String getUniqueName() const override;
    virtual void setName (const String& newName) override;
    virtual bool isMissing() override;
    virtual void showPopupMenu() override;
    virtual void handlePopupMenuResult (int) override;
    virtual Icon getIcon() const override;

    //void addSubItem();
    bool isRootAsset() const;
    void itemOpennessChanged (bool isNowOpen) override;

    // dragging stuff
    File getDraggableFile() const override
    {
        std::clog << "get draggable file\n";

        if (item.isFile())
            return item.getFile();

        return TreeItemBase::getDraggableFile();
    }

    // value tree
    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property) override;
    void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int) override;
    void valueTreeChildOrderChanged (ValueTree& parentTree, int, int) override;
    void valueTreeParentChanged (ValueTree& tree) override;

    AssetTree::Item item;

protected:
    void addSubItems() override;
    virtual AssetTreeViewItem* createAssetSubItem (const AssetTree::Item&) { return nullptr; }
    virtual void treeChildrenChanged (const ValueTree& parentTree);
    virtual void triggerAsyncAssetRename (const AssetTree::Item& item);
};

class PlainTextFileTreeViewItem : public AssetTreeViewItem
{
public:
    PlainTextFileTreeViewItem (const AssetTree::Item& item);
    ~PlainTextFileTreeViewItem();
    bool acceptsFileDrop (const StringArray&) const { return false; }
    bool acceptsDragItems (const OwnedArray<AssetTree::Item>&) { return false; }
    AssetTreeViewItem* createAssetSubItem (const AssetTree::Item& child);
    void showDocument();
    void showPopupMenu();
    void handlePopupMenuResult (int resultCode);
    String getDisplayName() const;
    void setName (const String& newName);
};

class GroupTreeViewItem : public AssetTreeViewItem
{
public:
    GroupTreeViewItem (const AssetTree::Item& item);
    virtual ~GroupTreeViewItem();

    bool isRootAsset() const { return item.isRoot(); }
    bool acceptsFileDrop (const StringArray&) const { return true; }
    bool acceptsDragItems (const OwnedArray<AssetTree::Item>& selectedNodes);
    void checkFileStatus();
    void moveSelectedItemsTo (OwnedArray<AssetTree::Item>& selectedNodes, int insertIndex);

    virtual void showDocument();
    virtual void showPopupMenu();
    virtual void handlePopupMenuResult (int resultCode);

    void addFiles (const StringArray& files, int insertIndex);
    void addNewGroup();

    void addCreateFileMenuItems (PopupMenu& m);
    void processCreateFileMenuItem (int item);

    Icon getIcon() const { return Icon (getIcons().folder, Colours::red); }

protected:
    AssetTreeViewItem* createAssetSubItem (const AssetTree::Item& child);
};

class AssetTreeView : public TreePanelBase
{
public:
    AssetTreeView (const AssetTree::Item& root);
    ~AssetTreeView() {}
};

} // namespace Element
#endif // ELEMENT_TREEVIEW_TYPES_H

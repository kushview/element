/*
    AssetTreeView.h - This file is part of Element
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

#ifndef ELEMENT_TREEVIEW_TYPES_H
#define ELEMENT_TREEVIEW_TYPES_H

class AssetTreeViewItem :  public Element::TreeItemBase,
                           public ValueTree::Listener
{
public:

    AssetTreeViewItem (const AssetItem& item);
    ~AssetTreeViewItem();

    virtual bool mightContainSubItems() override;
    virtual String getRenamingName() const override;
    virtual String getDisplayName() const override;
    virtual String getUniqueName() const override;
    virtual void setName (const String& newName) override;
    virtual bool isMissing() override;
    virtual void showPopupMenu() override;
    virtual void handlePopupMenuResult (int) override;
    virtual Element::Icon getIcon() const override;

    //void addSubItem();
    bool isRootAsset() const;
    void itemOpennessChanged (bool isNowOpen) override;

    // dragging stuff
    File getDraggableFile() const override
    {
        if (item.isFile())
            return item.getFile();

        return TreeItemBase::getDraggableFile();
    }

    // value tree
    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property) override;
    void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    void valueTreeChildOrderChanged (ValueTree& parentTree, int oldIndex, int newIndex) override;
    void valueTreeParentChanged (ValueTree& tree) override;

    AssetItem item;

protected:

    void addSubItems() override;
    virtual AssetTreeViewItem* createAssetSubItem (const AssetItem&) { return nullptr; }
    virtual void treeChildrenChanged (const ValueTree& parentTree);
    virtual void triggerAsyncAssetRename (const AssetItem& item);

};

class PlainTextFileTreeViewItem   : public AssetTreeViewItem
{
public:

    PlainTextFileTreeViewItem (const AssetItem& item);
    ~PlainTextFileTreeViewItem();
    bool acceptsFileDrop (const StringArray&) const { return false; }
    bool acceptsDragItems (const OwnedArray <AssetItem>&) { return false; }
    AssetTreeViewItem* createAssetSubItem (const AssetItem& child);
    void showDocument();
    void showPopupMenu();
    void handlePopupMenuResult (int resultCode);
    String getDisplayName() const;
    void setName (const String& newName);

};

class GroupTreeViewItem   : public AssetTreeViewItem
{
public:
    GroupTreeViewItem (const AssetItem& item);
    virtual ~GroupTreeViewItem();

    bool isRootAsset() const { return item.isRoot(); }
    bool acceptsFileDrop (const StringArray&) const { return true; }
    bool acceptsDragItems (const OwnedArray <AssetItem>& selectedNodes);
    void checkFileStatus();
    void moveSelectedItemsTo (OwnedArray <AssetItem>& selectedNodes, int insertIndex);

    virtual void showDocument();
    virtual void showPopupMenu();
    virtual void handlePopupMenuResult (int resultCode);

    void addFiles (const StringArray& files, int insertIndex);
    void addNewGroup();

    void addCreateFileMenuItems (PopupMenu& m);
    void processCreateFileMenuItem (int item);

    Icon getIcon() const { return Icon (getIcons().folder, Colours::red); }

protected:

    AssetTreeViewItem* createAssetSubItem (const AssetItem& child);

};

#endif   // ELEMENT_TREEVIEW_TYPES_H


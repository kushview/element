/*
    AssetTree.h - This file is part of Element
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

#ifndef ELEMENT_ASSET_TREE_H
#define ELEMENT_ASSET_TREE_H

#include "ElementApp.h"

namespace Element {
    
class RelativePath;

class AssetTree :  private ValueTree::Listener
{
public:

    class Item {
    public:

        Item (AssetTree& parent, const ValueTree& d)
            : data (d),
              tree (parent)
        { }

        Item (const Item& other)
            : data (other.data),
              tree (other.tree)
        { }

        inline bool operator== (const Item& other) const { return data == other.data && &tree == &other.tree; }
        bool operator!= (const Item& other) const { return ! operator== (other); }

        static Item createGroup (AssetTree& tree, const String& name, const String& uid);
        static Item touchFile (AssetTree& tree, const String& path, const String& uid);

        inline bool isFile()  const { return data.hasType ("file"); }
        inline bool isGroup() const { return data.hasType ("group") || isRoot(); }
        inline bool isRoot()  const { return data.hasType (tree.rootType()); }
        inline bool isValid() const { return data.isValid(); }

        inline String getName() const { return data.getProperty ("name", "Untitled"); }
        inline Value getNameValue()   { return data.getPropertyAsValue ("name", nullptr); }
        inline void setName (const String& name) { data.setProperty ("name", name, nullptr); }

        void setFile (const File& file);
        void setFile (const RelativePath& file);
        File getFile() const;
        String getFilePath() const;

        bool canContain (const Item& child) const;
        int getNumChildren() const { return data.getNumChildren(); }
        Item getChild (int index) const { return Item (tree, data.getChild (index)); }

        String getId() const { return data.getProperty ("id"); }
        void setId (const String& id) { data.setProperty ("id", id, nullptr); }

        Item findItemForFile (const File& file) const;
        Item findItemForId (const String& targetId) const;

        template<class ChildFunc>
        inline void foreachChild (ChildFunc& callback)
        {
            callback (*this);

            for (int i = getNumChildren(); --i >= 0;)
            {
                Item next (getChild (i));
                if (next.isValid())
                    next.foreachChild (callback);
            }
        }

        Item addNewSubGroup (const String& name, int insertIndex);
        void addChild (const Item& newChild, int insertIndex);
        bool addFile (const File& file, int insertIndex, bool shouldCompile);
        void addFileUnchecked (const File& file, int insertIndex, bool shouldCompile);
        //bool addRelativeFile (const RelativePath& file, int insertIndex, bool shouldCompile);

        File determineGroupFolder() const;

        Item getOrCreateSubGroup (const String& name);
        Item getParent() const;

        void removeFromTree();

        void sortAlphabetically (bool keepGroupsAtStart);
        // bool containsChildForFile (const RelativePath& file) const;

        void testPrint() const { std::clog << data.toXmlString() << std::endl; }

        ValueTree data; //XXX: this needs to be private

    private:

        friend class AssetTree;
        Item& operator = (const Item& other);


        AssetTree& tree;

        void setMissingProperties();
    };


    AssetTree (const ValueTree& parent, const String& rootName,
               const String& rootValType = "assets", UndoManager* u = nullptr);
    AssetTree (const String& rootName, const String& rootValType = "assets",
               UndoManager* u = nullptr);
    AssetTree (const AssetTree& other);

    ~AssetTree();

    /** Adds a group to the Root tree item */
    Item addGroup (const String& name);

    /** Adds an array of groups to the Root tree */
    void addGroups (const StringArray& groups, Array<Item>& result);

    /** Clear all entries from the tree */
    void clear();

    /** Create XML data for this tree */
    XmlElement* createXml() const;
    void loadFromXml (const XmlElement& xml);

    /** Get this trees root file/directory */
    const File& getFile() const;

    /** Find a group in the tree

        @param name The group name to find
        @param recursive Search the tree recursively
        @param createIt Create new group if not found
    */
    Item findGroup (const String& name, bool recursive = false, bool createIt = false);

    /** Returns a path for file relative to the tree's root */
    String getRelativePathForFile (const File& file) const;

    /** Return the root directory of this tree */
    File getRootDir() const;

    /** Get this tree's name */
    String name() const;
    String getName() const;

    /** Get this tree's name as a Element::Value */
    Value nameValue();

    /** Set the tree's root file/dir */
    void setFile (const File& file);

    /** Resolve a path to file */
    File resolveFilename (const String& filename);

    /** Returns the root item */
    Item root() const;

    /** The ValueTree type of this tree */
    const Identifier& rootType() const;

    /** Set an undo manager to use,
        Calling code must not delete the UndoManager before this tree
        is deleted
    */
    void setUndoManager (UndoManager* u);

    /** Get the undomanager */
    UndoManager* getUndoManager();


    void testPrint() const;

    /** Replace the ValueTree with new data */
    void setAssetsNode (const ValueTree& data);

protected:
    virtual void assetAdded (const AssetTree::Item&) { }
    virtual void assetRemoved (const AssetTree::Item&) { }

private:

    AssetTree& operator= (const AssetTree& other);

    UndoManager* undo;
    ValueTree assets;
    File manifestFile;
    const Identifier rootValueType;

    void addChildInternal (ValueTree& parent, ValueTree& child);
    Item createItem (const Identifier& type);
    bool hasParentValueTree() const;

    friend class ValueTree;
    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property);
    void valueTreeChildAdded (ValueTree& parent, ValueTree& child);
    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int indexOfRemoved);
    void valueTreeChildOrderChanged (ValueTree& parent, int newIndex, int oldIndex);
    void valueTreeParentChanged (ValueTree& child);

};

typedef AssetTree::Item AssetItem;
typedef OwnedArray<AssetItem> AssetArray;

}

#endif // ELEMENT_ASSET_TREE_H

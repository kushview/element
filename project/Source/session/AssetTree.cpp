/*
    AssetTree.cpp - This file is part of Element
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

#include "session/AssetTree.h"

namespace Element {

struct AssetItemSorterAlphabetical
{
    static int compareElements (const ValueTree& first, const ValueTree& second)
    {
        return first [Slugs::name].toString().
        compareIgnoreCase (second [Slugs::name].toString());
    }
};

struct AssetItemSorterWithGroupsAtStart
{
    static int compareElements (const ValueTree& first, const ValueTree& second)
    {
        const bool firstIsGroup = first.hasType (Slugs::group);
        const bool secondIsGroup = second.hasType (Slugs::group);
        
        if (firstIsGroup == secondIsGroup)
            return first [Slugs::name].toString().compareIgnoreCase (second [Slugs::name].toString());
        
        return firstIsGroup ? -1 : 1;
    }
};

void
AssetTree::Item::addChild (const Item& newChild, int insertIndex)
{
    data.addChild (newChild.data, insertIndex, nullptr);
}

void
AssetTree::Item::removeFromTree()
{
    data.getParent().removeChild (data, nullptr);
}

bool
AssetTree::Item::addFile (const File& file, int insertIndex, const bool shouldCompile)
{
    if (file == File::nonexistent || file.isHidden()
        || file.getFileName().startsWithChar ('.'))
        return false;
    
    if (file.isDirectory())
    {
        Item group (addNewSubGroup (file.getFileNameWithoutExtension(), insertIndex));
        for (DirectoryIterator iter (file, false, "*", File::findFilesAndDirectories); iter.next();)
            group.addFile (iter.getFile(), -1, shouldCompile);
        
        // xxx ! doesn't work // group.sortAlphabetically (false);
    }
    else if (file.existsAsFile())
    {
        //if (! tree.root().findItemForFile (file).isValid())
        addFileUnchecked (file, insertIndex, shouldCompile);
    }
    else
    {
        jassertfalse;
    }
    
    return true;
}

File AssetTree::Item::determineGroupFolder() const
{
    jassert (isGroup());
    File f;
    
    for (int i = 0; i < getNumChildren(); ++i)
    {
        f = getChild(i).getFile();
        
        if (f.exists())
            return f.getParentDirectory();
    }
    
    Item parent (getParent());
    if (parent != *this)
    {
        f = parent.determineGroupFolder();
        
        if (f.getChildFile (getName()).isDirectory())
            f = f.getChildFile (getName());
    }
    else
    {
        f = tree.getFile().getParentDirectory();
        
        if (f.getChildFile ("Source").isDirectory())
            f = f.getChildFile ("Source");
    }
    
    return f;
}


AssetTree::Item AssetTree::Item::getParent() const
{
    return Item (tree, data.getParent());
}

void AssetTree::Item::addFileUnchecked (const File& file, int insertIndex, const bool shouldCompile)
{
    Item item (tree, ValueTree (Slugs::file));
    item.setMissingProperties();
    item.getNameValue() = file.getFileName();
    
    if (canContain (item))
    {
        item.setFile (file);
        addChild (item, insertIndex);
    }
}

AssetTree::Item AssetTree::Item::addNewSubGroup (const String& name, int insertIndex)
{
    String newID (Utility::createGUID (getId() + name + String (getNumChildren())));
    
    int n = 0;
    while (tree.root().findItemForId (newID).isValid())
        newID = Utility::createGUID (newID + String (++n));
    
    Item group (createGroup (tree, name, newID));
    
    jassert (canContain (group));
    addChild (group, insertIndex);
    return group;
}

bool AssetTree::Item::canContain (const Item& child) const
{
    if (isFile())
        return false;
    
    if (isGroup())
        return child.isFile() || child.isGroup();
    
    jassertfalse;
    return false;
}

AssetTree::Item AssetTree::Item::createGroup (AssetTree& tree, const String& name, const String& uid)
{
    Item group (tree, ValueTree (Slugs::group));
    group.setId (uid);
    group.setMissingProperties();
    group.getNameValue() = name;
    return group;
}

String AssetTree::Item::getFilePath() const
{
    if (isFile())
        return data.getProperty(Slugs::path).toString();
    
    return String::empty;
}

File AssetTree::Item::getFile() const
{
    if (isFile())
        return tree.resolveFilename (data.getProperty(Slugs::path).toString());
    
    return File::nonexistent;
}

AssetTree::Item AssetTree::Item::findItemForFile (const File& file) const
{
    if (getFile() == file)
        return *this;
    
    if (isGroup())
    {
        for (int i = getNumChildren(); --i >= 0;)
        {
            Item found (getChild(i).findItemForFile (file));
            if (found.isValid())
                return found;
        }
    }
    
    return AssetTree::Item (tree, ValueTree::invalid);
}


AssetTree::Item AssetTree::Item::findItemForId (const String &targetId) const
{
    
    if (data [Slugs::id] == targetId) {
        return *this;
    }
    
    if (isGroup())
    {
        for (int i = getNumChildren(); --i >= 0;)
        {
            Item found (getChild(i).findItemForId (targetId));
            if (found.isValid())
                return found;
        }
    }
    
    return AssetTree::Item (tree, ValueTree::invalid);
}

void AssetTree::Item::sortAlphabetically (bool keepGroupsAtStart)
{
    if (keepGroupsAtStart)
    {
        AssetItemSorterWithGroupsAtStart sorter;
        data.sort (sorter, nullptr, true);
    }
    else
    {
        AssetItemSorterAlphabetical sorter;
        data.sort (sorter, nullptr, true);
    }
}

void AssetTree::Item::setFile (const File& file)
{
    setFile (RelativePath (tree.getRelativePathForFile (file), RelativePath::projectFolder));
    jassert (getFile() == file);
}

void AssetTree::Item::setFile (const RelativePath& file)
{
    jassert (file.getRoot() == RelativePath::projectFolder);
    jassert (isFile());
    data.setProperty (Slugs::path, file.toUnixStyle(), nullptr);
    data.setProperty (Slugs::name, file.getFileName(), nullptr);
}

void AssetTree::Item::setMissingProperties()
{
    if (isFile() && ! data.hasProperty (Slugs::id))
    {
        String id (Utility::createAlphaNumericUID());
        while (tree.root().findItemForId(id).isValid())
            id = Utility::createAlphaNumericUID();
        
        setId (id);
    }
}



AssetTree::AssetTree (const ValueTree& parent, const String& rootName,
                      const String& rootValType, UndoManager* u)
: undo (u),
assets (rootValType),
rootValueType (rootValType)

{
    assets.setProperty ("name", rootName, nullptr);
    
    ValueTree(parent).addChild (assets, -1, nullptr);
    
    root().setId (Utility::createGUID (rootName + rootValType));
    assets.addListener(this);
}

AssetTree::AssetTree (const String& rootName, const String& rootValType, UndoManager* u)
    : undo (u),
      assets (rootValType),
      rootValueType (rootValType)
{
    assets.setProperty (Slugs::name, rootName, nullptr);
    root().setId (Utility::createGUID (rootName + rootValType));
    assets.addListener (this);
}

AssetTree::AssetTree (const AssetTree& other)
    : undo (other.undo),
      assets (other.assets)
{
    assets.addListener(this);
}

AssetTree::~AssetTree() { assets.removeListener (this); }

void AssetTree::addChildInternal (ValueTree& parent, ValueTree& child)
{
    parent.addChild (child, -1, undo);
}



AssetTree::Item AssetTree::addGroup (const String& name)
{
    Item item (createItem ("group"));
    item.getNameValue() = name;
    item.setId (Utility::createAlphaNumericUID ());
    addChildInternal (assets, item.data);
    
    return item;
}

void AssetTree::addGroups (const StringArray& groups, Array<Item>& result)
{
	for (int i = 0; i < groups.size(); ++i)
        result.add (addGroup (groups [i]));
}

void AssetTree::clear()
{
    assets.removeAllChildren (undo);
}


AssetTree::Item AssetTree::createItem (const Identifier& type)
{
    ValueTree newData (type);
    return Item (*this, newData);
}

AssetTree::Item AssetTree::findGroup (const String& name, bool recursive, bool createIt)
{
    if (! recursive)
    {
        for (int i = assets.getNumChildren(); --i >= 0;)
        {
            ValueTree vt (assets.getChild (i));
            Item item (*this, vt);
            if (! item.isGroup())
                continue;
            
            if (name == item.getName())
                return item;
        }
    }
    
    if (createIt)
        return addGroup (name);
    
    return Item  (*this, ValueTree::invalid);
    
}


String AssetTree::getRelativePathForFile (const File& file) const
{
    String filename (file.getFullPathName());
    
    File relativePathBase (getFile().getParentDirectory());
    
    String p1 (relativePathBase.getFullPathName());
    String p2 (file.getFullPathName());
    
    while (p1.startsWithChar (File::separator))
        p1 = p1.substring (1);
    
    while (p2.startsWithChar (File::separator))
        p2 = p2.substring (1);
    
    if (p1.upToFirstOccurrenceOf (File::separatorString, true, false)
        .equalsIgnoreCase (p2.upToFirstOccurrenceOf (File::separatorString, true, false)))
    {
        filename = FileHelpers::getRelativePathFrom (file, relativePathBase);
    }
    
    return filename;
}

bool AssetTree::hasParentValueTree() const
{
    return assets.getParent().isValid();
}

String AssetTree::name() const
{
    jassertfalse;
    return this->getName();
}

String AssetTree::getName() const
{
    return assets.getProperty ("name", "Asset Tree");
}

Value AssetTree::nameValue()
{
    return assets.getPropertyAsValue ("name", nullptr);
}

const File& AssetTree::getFile() const
{
    return manifestFile;
}

UndoManager* AssetTree::getUndoManager()
{
    return undo;
}

File AssetTree::getRootDir() const
{
    if (! manifestFile.isDirectory())
        return manifestFile.getParentDirectory();
    return manifestFile;
}

File AssetTree::resolveFilename (const String& filename)
{
    if (filename.isEmpty())
        return File::nonexistent;
    
    return getFile().getSiblingFile (FileHelpers::currentOSStylePath (filename));
}

AssetTree::Item AssetTree::root() const
{
    return Item (*const_cast<AssetTree*> (this), this->assets);
}

const Identifier& AssetTree::rootType() const
{
    return rootValueType;
}

void AssetTree::setFile (const File& file)
{
    manifestFile = file;
}

void AssetTree::setUndoManager (UndoManager* u)
{
    undo = u;
}

void AssetTree::setAssetsNode (const ValueTree& data)
{
    if (data == assets)
        return;
    assets = data;
}

void AssetTree::testPrint() const
{
    std::clog << assets.toXmlString() << std::endl;
}

void AssetTree::valueTreeChildAdded (ValueTree& parent, ValueTree& child)
{
    if (parent != assets)
        return;
    
    Item item (*this, child);
    assetAdded (item);
}

void AssetTree::valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int /*indexOfRemoved*/)
{
    if (parent != assets)
        return;
    
    Item item (*this, child);
    assetRemoved (item);
}

void AssetTree::valueTreeChildOrderChanged (ValueTree& /*parent*/, int /*oldIndex*/, int /*newIndex*/) { }
void AssetTree::valueTreeParentChanged (ValueTree& /* child */) { }
void AssetTree::valueTreePropertyChanged (ValueTree& /* tree */, const Identifier& /* property */) { }

XmlElement*
AssetTree::createXml() const
{
    return this->assets.createXml();
}

void
AssetTree::loadFromXml (const XmlElement& xml)
{
    this->assets = ValueTree::fromXml (xml);
}

}

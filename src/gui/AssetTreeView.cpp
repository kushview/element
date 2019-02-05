/*
    AssetTreeView.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "AssetTreeView.h"

namespace Element {

AssetTreeViewItem::AssetTreeViewItem (const AssetItem& i)
    : item (i)
{
    item.data.addListener (this);
}

AssetTreeViewItem::~AssetTreeViewItem()
{
    item.data.addListener (this);
}

void AssetTreeViewItem::addSubItems()
{
    for (int i = 0; i < item.getNumChildren(); ++i)
        if (AssetTreeViewItem* p = createAssetSubItem (item.getChild (i)))
            addSubItem (p);
}

bool AssetTreeViewItem::mightContainSubItems() { return item.isGroup(); }
String AssetTreeViewItem::getRenamingName() const { return item.getName(); }
String AssetTreeViewItem::getDisplayName()  const { return item.getName(); }
String AssetTreeViewItem::getUniqueName()   const { return item.getId(); }
bool AssetTreeViewItem::isMissing() { return false; }
Icon AssetTreeViewItem::getIcon() const { return Icon(); }
void AssetTreeViewItem::setName (const String& newName) { item.setName (newName); }
void AssetTreeViewItem::showPopupMenu() { }
void AssetTreeViewItem::handlePopupMenuResult (int res) { }
void AssetTreeViewItem::itemOpennessChanged (bool isNowOpen)
{
    if (isNowOpen) {
        refreshSubItems();
    }
}

bool AssetTreeViewItem::isRootAsset() const { return item.isRoot(); }

void AssetTreeViewItem::triggerAsyncAssetRename (const AssetTree::Item &itemToRename)
{
    class RenameMessage  : public CallbackMessage
    {
    public:
        RenameMessage (TreeView* const t, const AssetTree::Item& i)
            : tree (t), itemToRename (i)  {}

        void messageCallback() override
        {
#if 0
            if (tree != nullptr)
                if (AssetTreeViewItem* root = dynamic_cast <AssetTreeViewItem*> (tree->getRootItem()))
                    if (AssetTreeViewItem* found = root->findTreeViewItem (itemToRename))
                        found->showRenameBox();
#endif
        }

    private:
        Component::SafePointer<TreeView> tree;
        AssetTree::Item itemToRename;
    };

    (new RenameMessage (getOwnerView(), itemToRename))->post();
}



//==============================================================================
void AssetTreeViewItem::treeChildrenChanged (const ValueTree& parentTree)
{
    if (parentTree == item.data)
    {
        refreshSubItems();
        treeHasChanged();
        setOpen (true);
    }
}

void AssetTreeViewItem::valueTreePropertyChanged (ValueTree& tree, const Identifier&)
{
    if (tree == item.data)
        repaintItem();
}

void AssetTreeViewItem::valueTreeChildAdded (ValueTree& parent, ValueTree& child)
{
    treeChildrenChanged (parent);
}

void AssetTreeViewItem::valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int)
{
    treeChildrenChanged (parent);
}

void AssetTreeViewItem::valueTreeChildOrderChanged (ValueTree& parent, int, int)
{
    treeChildrenChanged (parent);
}

void AssetTreeViewItem::valueTreeParentChanged (ValueTree& v)
{
}

//==============================================================================
GroupTreeViewItem::GroupTreeViewItem (const AssetTree::Item& i)
    : AssetTreeViewItem (i)
{ }

GroupTreeViewItem::~GroupTreeViewItem()
{
}

void GroupTreeViewItem::addNewGroup()
{
    AssetTree::Item newGroup (item.addNewSubGroup ("New Group", 0));
    triggerAsyncAssetRename (newGroup);
}

bool GroupTreeViewItem::acceptsDragItems (const OwnedArray <AssetTree::Item>& selectedNodes)
{
    for (int i = selectedNodes.size(); --i >= 0;)
        if (item.canContain (*selectedNodes.getUnchecked(i)))
            return true;

    return false;
}

void GroupTreeViewItem::addFiles (const StringArray& files, int insertIndex)
{
    for (int i = 0; i < files.size(); ++i)
    {
        const File file (files[i]);

        if (item.addFile (file, insertIndex, true))
            ++insertIndex;
    }
}

void GroupTreeViewItem::moveSelectedItemsTo (OwnedArray <AssetTree::Item>& selectedNodes, int insertIndex)
{
   // moveItems (selectedNodes, item, insertIndex);
}

void GroupTreeViewItem::checkFileStatus()
{
 //   for (int i = 0; i < getNumSubItems(); ++i)
  //      if (AssetTreeViewItem* p = dynamic_cast <AssetTreeViewItem*> (getSubItem(i)))
   //         p->checkFileStatus();
}

AssetTreeViewItem*
GroupTreeViewItem::createAssetSubItem (const AssetTree::Item& child)
{
    if (child.isGroup())   return new GroupTreeViewItem (child);
    if (child.isFile())    return new PlainTextFileTreeViewItem (child);

    jassertfalse;
    return nullptr;
}

void GroupTreeViewItem::showDocument()
{
//    if (ProjectContentComponent* pcc = getProjectContentComponent())
     //   pcc->setEditorComponent (new GroupInformationComponent (item), nullptr);
}

static void openOrCloseAllSubGroups (TreeViewItem& item, bool shouldOpen)
{
    item.setOpen (shouldOpen);

    for (int i = item.getNumSubItems(); --i >= 0;)
        if (TreeViewItem* sub = item.getSubItem(i))
            openOrCloseAllSubGroups (*sub, shouldOpen);
}

static void setFilesToCompile (AssetTree::Item item, const bool shouldCompile)
{
   // if (item.isFile())
       // item.getShouldCompileValue() = shouldCompile;
//
  //  for (int i = item.getNumChildren(); --i >= 0;)
      //  setFilesToCompile (item.getChild (i), shouldCompile);
}

void GroupTreeViewItem::showPopupMenu()
{
    PopupMenu m;
    addCreateFileMenuItems (m);

    m.addSeparator();

    if (isOpen())
        m.addItem (1, "Collapse all Sub-groups");
    else
        m.addItem (2, "Expand all Sub-groups");

    m.addSeparator();
    m.addItem (3, "Enable compiling of all enclosed files");
    m.addItem (4, "Disable compiling of all enclosed files");

    m.addSeparator();
    m.addItem (5, "Sort Items Alphabetically");
    m.addItem (6, "Sort Items Alphabetically (Groups first)");
    m.addSeparator();
    m.addItem (7, "Rename...");

    if (! isRootAsset())
        m.addItem (8, "Delete");

    launchPopupMenu (m);
}

void GroupTreeViewItem::handlePopupMenuResult (int resultCode)
{
    switch (resultCode)
    {
        case 1:     openOrCloseAllSubGroups (*this, false); break;
        case 2:     openOrCloseAllSubGroups (*this, true); break;
        case 3:     setFilesToCompile (item, true); break;
        case 4:     setFilesToCompile (item, false); break;
        case 5:     item.sortAlphabetically (false); break;
        case 6:     item.sortAlphabetically (true); break;
        case 7:     triggerAsyncAssetRename (item); break;
        case 8:     deleteAllSelectedItems(); break;
        default:    processCreateFileMenuItem (resultCode); break;
    }
}

void GroupTreeViewItem::addCreateFileMenuItems (PopupMenu& m)
{
    //m.addItem (1001, "Add New Group");
    //m.addItem (1002, "Add Existing Files...");

    //m.addSeparator();
    //NewFileWizard().addWizardsToMenu (m);
}

void GroupTreeViewItem::processCreateFileMenuItem (int menuID)
{
#if 0
    switch (menuID)
    {
        case 1001:  addNewGroup(); break;
        case 1002:  browseToAddExistingFiles(); break;

        default:
            NewFileWizard().runWizardFromMenu (menuID, item);
            break;
    }
#endif
}


//==============================================================================
//==============================================================================
PlainTextFileTreeViewItem::PlainTextFileTreeViewItem (const AssetTree::Item& item_)
    : AssetTreeViewItem (item_)
{
}

PlainTextFileTreeViewItem::~PlainTextFileTreeViewItem()
{
}

String PlainTextFileTreeViewItem::getDisplayName() const
{
    return item.getName();
}

void PlainTextFileTreeViewItem::setName (const String& newName)
{
#if 0
    if (newName != File::createLegalFileName (newName))
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon, "File Rename",
                                     "That filename contained some illegal characters!");
        triggerAsyncRename (item);
        return;
    }

    File oldFile (getFile());
    File newFile (oldFile.getSiblingFile (newName));
    File correspondingFile (findCorrespondingHeaderOrCpp (oldFile));

    if (correspondingFile.exists() && newFile.hasFileExtension (oldFile.getFileExtension()))
    {
        AssetTree::Item correspondingItem (item.project.getMainGroup().findItemForFile (correspondingFile));

        if (correspondingItem.isValid())
        {
            if (AlertWindow::showOkCancelBox (AlertWindow::NoIcon, "File Rename",
                                              "Do you also want to rename the corresponding file \"" + correspondingFile.getFileName()
                                                + "\" to match?"))
            {
                if (! item.renameFile (newFile))
                {
                    AlertWindow::showMessageBox (AlertWindow::WarningIcon, "File Rename",
                                                 "Failed to rename \"" + oldFile.getFullPathName() + "\"!\n\nCheck your file permissions!");
                    return;
                }

                if (! correspondingItem.renameFile (newFile.withFileExtension (correspondingFile.getFileExtension())))
                {
                    AlertWindow::showMessageBox (AlertWindow::WarningIcon, "File Rename",
                                                 "Failed to rename \"" + correspondingFile.getFullPathName() + "\"!\n\nCheck your file permissions!");
                }
            }
        }
    }

    if (! item.renameFile (newFile))
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon, "File Rename",
                                     "Failed to rename the file!\n\nCheck your file permissions!");
    }
#endif
}

AssetTreeViewItem* PlainTextFileTreeViewItem::createAssetSubItem (const AssetTree::Item&)
{
    jassertfalse;
    return nullptr;
}

void PlainTextFileTreeViewItem::showDocument()
{
#if 0
    const File f (getFile());

    if (f.exists())
        if (ProjectContentComponent* pcc = getProjectContentComponent())
            pcc->showEditorForFile (f, false);
#endif
}

void PlainTextFileTreeViewItem::showPopupMenu()
{
    PopupMenu m;
#if 0
    if (GroupTreeViewItem* parentGroup = dynamic_cast <GroupTreeViewItem*> (getParentProjectItem()))
    {
        parentGroup->addCreateFileMenuItems (m);
        m.addSeparator();
    }

    m.addItem (1, "Open in external editor");
    m.addItem (2,
                 #if JUCE_MAC
                  "Reveal in Finder");
                 #else
                  "Reveal in Explorer");
                 #endif

    m.addItem (4, "Rename File...");
    m.addSeparator();
    m.addItem (3, "Delete");
#endif

    launchPopupMenu (m);
}

void PlainTextFileTreeViewItem::handlePopupMenuResult (int resultCode)
{
#if 0
    switch (resultCode)
    {
        case 1:     getFile().startAsProcess(); break;
        case 2:     revealInFinder(); break;
        case 3:     deleteAllSelectedItems(); break;
        case 4:     triggerAsyncRename (item); break;

        default:
            if (GroupTreeViewItem* parentGroup = dynamic_cast <GroupTreeViewItem*> (getParentProjectItem()))
                parentGroup->processCreateFileMenuItem (resultCode);

            break;
    }
#endif
}


AssetTreeView::AssetTreeView (const AssetTree::Item& root)
    : TreePanelBase ("assets")
{
    setRoot (new AssetTreeViewItem (root));
}

}

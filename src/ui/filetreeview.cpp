// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "ui/filetreeview.hpp"

namespace juce {
extern Image juce_createIconForFile (const File& file);
}

using namespace juce;

namespace element {

//==============================================================================
class FileListTreeItem : public TreeViewItem,
                         private TimeSliceClient,
                         private AsyncUpdater,
                         private ChangeListener
{
public:
    FileListTreeItem (FileTreeView& treeComp,
                      DirectoryContentsList* parentContents,
                      int indexInContents,
                      const File& f,
                      TimeSliceThread& t)
        : file (f),
          owner (treeComp),
          parentContentsList (parentContents),
          indexInContentsList (indexInContents),
          subContentsList (nullptr, false),
          thread (t)
    {
        DirectoryContentsList::FileInfo fileInfo;

        if (parentContents != nullptr
            && parentContents->getFileInfo (indexInContents, fileInfo))
        {
            fileSize = File::descriptionOfSizeInBytes (fileInfo.fileSize);
            modTime = fileInfo.modificationTime.formatted ("%d %b '%y %H:%M");
            isDirectory = fileInfo.isDirectory;
        }
        else
        {
            isDirectory = true;
        }
    }

    ~FileListTreeItem() override
    {
        thread.removeTimeSliceClient (this);
        clearSubItems();
        removeSubContentsList();
    }

    //==============================================================================
    bool mightContainSubItems() override { return isDirectory; }
    String getUniqueName() const override { return file.getFullPathName(); }
    int getItemHeight() const override { return owner.getItemHeight(); }

    var getDragSourceDescription() override
    {
        return owner.dragsNativeFiles()
                   ? var()
                   : var (owner.getDragAndDropDescription());
    }

    void itemOpennessChanged (bool isNowOpen) override
    {
        if (isNowOpen)
        {
            clearSubItems();

            isDirectory = file.isDirectory();

            if (isDirectory)
            {
                if (subContentsList == nullptr && parentContentsList != nullptr)
                {
                    auto l = new DirectoryContentsList (parentContentsList->getFilter(), thread);

                    l->setDirectory (file,
                                     parentContentsList->isFindingDirectories(),
                                     parentContentsList->isFindingFiles());

                    setSubContentsList (l, true);
                }

                changeListenerCallback (nullptr);
            }
        }
    }

    void removeSubContentsList()
    {
        if (subContentsList != nullptr)
        {
            subContentsList->removeChangeListener (this);
            subContentsList.reset();
        }
    }

    void setSubContentsList (DirectoryContentsList* newList, const bool canDeleteList)
    {
        removeSubContentsList();

        subContentsList = OptionalScopedPointer<DirectoryContentsList> (newList, canDeleteList);
        newList->addChangeListener (this);
    }

    bool selectFile (const File& target)
    {
        if (file == target)
        {
            setSelected (true, true);
            return true;
        }

        if (target.isAChildOf (file))
        {
            setOpen (true);

            for (int maxRetries = 500; --maxRetries > 0;)
            {
                for (int i = 0; i < getNumSubItems(); ++i)
                    if (auto* f = dynamic_cast<FileListTreeItem*> (getSubItem (i)))
                        if (f->selectFile (target))
                            return true;

                // if we've just opened and the contents are still loading, wait for it..
                if (subContentsList != nullptr && subContentsList->isStillLoading())
                {
                    Thread::sleep (10);
                    rebuildItemsFromContentList();
                }
                else
                {
                    break;
                }
            }
        }

        return false;
    }

    void changeListenerCallback (ChangeBroadcaster*) override
    {
        rebuildItemsFromContentList();
    }

    void rebuildItemsFromContentList()
    {
        clearSubItems();

        if (isOpen() && subContentsList != nullptr)
        {
            for (int i = 0; i < subContentsList->getNumFiles(); ++i)
                addSubItem (new FileListTreeItem (owner, subContentsList, i, subContentsList->getFile (i), thread));
        }
    }

    void paintItem (Graphics& g, int width, int height) override
    {
        ScopedLock lock (iconUpdate);

        if (file != File())
        {
            updateIcon (true);

            if (icon.isNull())
                thread.addTimeSliceClient (this);
        }

        owner.getLookAndFeel().drawFileBrowserRow (g, width, height, file, file.getFileName(), &icon, fileSize, modTime, isDirectory, isSelected(), indexInContentsList, owner);
    }

    std::unique_ptr<Component> createItemComponent() override
    {
        return owner.dragsNativeFiles()
                   ? std::make_unique<ItemComponent> (*this)
                   : nullptr;
    };

    String getAccessibilityName() override
    {
        return file.getFileName();
    }

    void itemClicked (const MouseEvent& e) override
    {
        owner.sendMouseClickMessage (file, e);
    }

    void itemDoubleClicked (const MouseEvent& e) override
    {
        TreeViewItem::itemDoubleClicked (e);

        owner.sendDoubleClickMessage (file);
    }

    void itemSelectionChanged (bool) override
    {
        owner.sendSelectionChangeMessage();
    }

    int useTimeSlice() override
    {
        updateIcon (false);
        return -1;
    }

    void handleAsyncUpdate() override
    {
        owner.repaint();
    }

    const File file;

private:
    FileTreeView& owner;
    DirectoryContentsList* parentContentsList;
    int indexInContentsList;
    OptionalScopedPointer<DirectoryContentsList> subContentsList;
    bool isDirectory;
    TimeSliceThread& thread;
    CriticalSection iconUpdate;
    Image icon;
    String fileSize, modTime;

    class ItemComponent : public Component
    {
    public:
        ItemComponent (FileListTreeItem& i)
            : item (i) {}

        void paint (Graphics& g) override
        {
            item.paintItem (g, getWidth(), getHeight());
        }

        void mouseDoubleClick (const MouseEvent& ev) override
        {
            item.itemDoubleClicked (ev);
        }

        void mouseDrag (const MouseEvent& ev) override
        {
            if (dragging)
                return;

            StringArray files;
            files.add (item.file.getFullPathName());
            for (int i = 0; i < item.owner.getNumSelectedFiles(); ++i)
            {
                auto f = item.owner.getSelectedFile (i);
                files.add (f.getFullPathName());
            }

            files.removeDuplicates (false);
            files.removeEmptyStrings();
            dragging = DragAndDropContainer::performExternalDragDropOfFiles (
                files, false, this, [this]() { dragging = false; });
        }

        void mouseUp (const MouseEvent& event) override
        {
            if (! dragging && getLocalBounds().contains (event.position.toInt()))
            {
                item.itemClicked (event);
                item.setSelected (true, true);
            }
            dragging = false;
        }

    private:
        FileListTreeItem& item;
        bool dragging = false;
    };

    void updateIcon (const bool onlyUpdateIfCached)
    {
        if (icon.isNull())
        {
            auto hashCode = (file.getFullPathName() + "_iconCacheSalt").hashCode();
            auto im = ImageCache::getFromHashCode (hashCode);

            if (im.isNull() && ! onlyUpdateIfCached)
            {
                im = juce::juce_createIconForFile (file);

                if (im.isValid())
                    ImageCache::addImageToCache (im, hashCode);
            }

            if (im.isValid())
            {
                {
                    ScopedLock lock (iconUpdate);
                    icon = im;
                }

                triggerAsyncUpdate();
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileListTreeItem)
};

//==============================================================================
FileTreeView::FileTreeView (DirectoryContentsList& listToShow)
    : DirectoryContentsDisplayComponent (listToShow),
      itemHeight (22)
{
    setRootItemVisible (false);
    refresh();
}

FileTreeView::~FileTreeView()
{
    deleteRootItem();
}

void FileTreeView::refresh()
{
    deleteRootItem();

    auto root = new FileListTreeItem (*this, nullptr, 0, directoryContentsList.getDirectory(), directoryContentsList.getTimeSliceThread());

    root->setSubContentsList (&directoryContentsList, false);
    setRootItem (root);
}

//==============================================================================
File FileTreeView::getSelectedFile (const int index) const
{
    if (auto* item = dynamic_cast<const FileListTreeItem*> (getSelectedItem (index)))
        return item->file;

    return {};
}

void FileTreeView::deselectAllFiles()
{
    clearSelectedItems();
}

void FileTreeView::scrollToTop()
{
    getViewport()->getVerticalScrollBar().setCurrentRangeStart (0);
}

void FileTreeView::setDragAndDropDescription (const String& description)
{
    dragAndDropDescription = description;
}

void FileTreeView::setSelectedFile (const File& target)
{
    if (auto* t = dynamic_cast<FileListTreeItem*> (getRootItem()))
        if (! t->selectFile (target))
            clearSelectedItems();
}

void FileTreeView::setItemHeight (int newHeight)
{
    if (itemHeight != newHeight)
    {
        itemHeight = newHeight;

        if (auto* root = getRootItem())
            root->treeHasChanged();
    }
}

//============================================================================
void FileTreeView::setDragNativeFiles (bool useNative)
{
    if (nativeFileDrag == useNative)
        return;

    nativeFileDrag = useNative;
    refresh();
}

} // namespace element

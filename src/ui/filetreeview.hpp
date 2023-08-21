// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "element/juce.hpp"

namespace element {

//==============================================================================
/**
    A component that displays the files in a directory as a treeview.

    This implements the DirectoryContentsDisplayComponent base class so that
    it can be used in a FileBrowserComponent.

    To attach a listener to it, use its DirectoryContentsDisplayComponent base
    class and the FileBrowserListener class.

    @see DirectoryContentsList, FileListComponent

    @tags{GUI}
*/

class JUCE_API FileTreeView : public juce::TreeView,
                              public juce::DirectoryContentsDisplayComponent
{
public:
    //==============================================================================
    /** Creates a listbox to show the contents of a specified directory.
    */
    FileTreeView (juce::DirectoryContentsList& listToShow);

    /** Destructor. */
    ~FileTreeView() override;

    //==============================================================================
    /** Returns the number of files the user has got selected.
        @see getSelectedFile
    */
    int getNumSelectedFiles() const override { return juce::TreeView::getNumSelectedItems(); }

    /** Returns one of the files that the user has currently selected.
        The index should be in the range 0 to (getNumSelectedFiles() - 1).
        @see getNumSelectedFiles
    */
    juce::File getSelectedFile (int index = 0) const override;

    /** Deselects any files that are currently selected. */
    void deselectAllFiles() override;

    /** Scrolls the list to the top. */
    void scrollToTop() override;

    /** If the specified file is in the list, it will become the only selected item
        (and if the file isn't in the list, all other items will be deselected). */
    void setSelectedFile (const juce::File&) override;

    /** Updates the files in the list. */
    void refresh();

    /** Setting a name for this allows tree items to be dragged.

        The string that you pass in here will be returned by the getDragSourceDescription()
        of the items in the tree. For more info, see TreeViewItem::getDragSourceDescription().
    */
    void setDragAndDropDescription (const juce::String& description);

    /** Returns the last value that was set by setDragAndDropDescription().
    */
    const juce::String& getDragAndDropDescription() const noexcept { return dragAndDropDescription; }

    /** Changes the height of the treeview items. */
    void setItemHeight (int newHeight);

    /** Returns the height of the treeview items. */
    int getItemHeight() const noexcept { return itemHeight; }

    /** Do native file drag */
    void setDragNativeFiles (bool useNative);

    /** Returns true if dragging native files */
    bool dragsNativeFiles() const noexcept { return nativeFileDrag; }

private:
    //==============================================================================
    juce::String dragAndDropDescription;
    int itemHeight;
    bool nativeFileDrag = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileTreeView)
};

} // namespace element

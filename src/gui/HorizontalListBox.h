#pragma once

#include "JuceHeader.h"

/**
    A horizontal list of items that can be scrolled vertically.

    To create a list, you'll need to create a subclass of ListBoxModel. This can
    either paint each row of the list and respond to events via callbacks, or for
    more specialised tasks, it can supply a custom component to fill each row.

    @see juce::ComboBox, juce::TableListBox
*/

class HorizontalListBox  : public Component,
                           public SettableTooltipClient
{
public:
    
    /** Creates a ListBox.
     
     The model pointer passed-in can be null, in which case you can set it later
     with setModel().
     */
    HorizontalListBox (const String& componentName = String::empty,
                       ListBoxModel* model = nullptr);
    
    /** Destructor. */
    ~HorizontalListBox();
    
    
    
    /** Changes the current data model to display. */
    void setModel (ListBoxModel* newModel);
    
    /** Returns the current list model. */
    ListBoxModel* getModel() const noexcept                     { return model; }
    
    
    
    /** Causes the list to refresh its content.
     
     Call this when the number of rows in the list changes, or if you want it
     to call refreshComponentForRow() on all the row components.
     
     This must only be called from the main message thread.
     */
    void updateContent();
    
    
    /** Turns on multiple-selection of rows.
     
     By default this is disabled.
     
     When your row component gets clicked you'll need to call the
     selectRowsBasedOnModifierKeys() method to tell the list that it's been
     clicked and to get it to do the appropriate selection based on whether
     the ctrl/shift keys are held down.
     */
    void setMultipleSelectionEnabled (bool shouldBeEnabled) noexcept;
    
    /** If enabled, this makes the listbox flip the selection status of
     each row that the user clicks, without affecting other selected rows.
     
     (This only has an effect if multiple selection is also enabled).
     If not enabled, you can still get the same row-flipping behaviour by holding
     down CMD or CTRL when clicking.
     */
    void setClickingTogglesRowSelection (bool flipRowSelection) noexcept;
    
    /** Makes the list react to mouse moves by selecting the row that the mouse if over.
     
     This function is here primarily for the ComboBox class to use, but might be
     useful for some other purpose too.
     */
    void setMouseMoveSelectsRows (bool shouldSelect);
    
    
    /** Selects a row.
     
     If the row is already selected, this won't do anything.
     
     @param rowNumber                the row to select
     @param dontScrollToShowThisRow  if true, the list's position won't change; if false and
     the selected row is off-screen, it'll scroll to make
     sure that row is on-screen
     @param deselectOthersFirst      if true and there are multiple selections, these will
     first be deselected before this item is selected
     @see isRowSelected, selectRowsBasedOnModifierKeys, flipRowSelection, deselectRow,
     deselectAllRows, selectRangeOfRows
     */
    void selectRow (int rowNumber,
                    bool dontScrollToShowThisRow = false,
                    bool deselectOthersFirst = true);
    
    /** Selects a set of rows.
     
     This will add these rows to the current selection, so you might need to
     clear the current selection first with deselectAllRows()
     
     @param firstRow     the first row to select (inclusive)
     @param lastRow      the last row to select (inclusive)
     */
    void selectRangeOfRows (int firstRow,
                            int lastRow);
    
    /** Deselects a row.
     If it's not currently selected, this will do nothing.
     @see selectRow, deselectAllRows
     */
    void deselectRow (int rowNumber);
    
    /** Deselects any currently selected rows.
     @see deselectRow
     */
    void deselectAllRows();
    
    /** Selects or deselects a row.
     If the row's currently selected, this deselects it, and vice-versa.
     */
    void flipRowSelection (int rowNumber);
    
    /** Returns a sparse set indicating the rows that are currently selected.
     @see setSelectedRows
     */
    SparseSet<int> getSelectedRows() const;
    
    /** Sets the rows that should be selected, based on an explicit set of ranges.
     
     If sendNotificationEventToModel is true, the ListBoxModel::selectedRowsChanged()
     method will be called. If it's false, no notification will be sent to the model.
     
     @see getSelectedRows
     */
    void setSelectedRows (const SparseSet<int>& setOfRowsToBeSelected,
                          NotificationType sendNotificationEventToModel = sendNotification);
    
    /** Checks whether a row is selected.
     */
    bool isRowSelected (int rowNumber) const;
    
    /** Returns the number of rows that are currently selected.
     @see getSelectedRow, isRowSelected, getLastRowSelected
     */
    int getNumSelectedRows() const;
    
    /** Returns the row number of a selected row.
     
     This will return the row number of the Nth selected row. The row numbers returned will
     be sorted in order from low to high.
     
     @param index    the index of the selected row to return, (from 0 to getNumSelectedRows() - 1)
     @returns        the row number, or -1 if the index was out of range or if there aren't any rows
     selected
     @see getNumSelectedRows, isRowSelected, getLastRowSelected
     */
    int getSelectedRow (int index = 0) const;
    
    /** Returns the last row that the user selected.
     
     This isn't the same as the highest row number that is currently selected - if the user
     had multiply-selected rows 10, 5 and then 6 in that order, this would return 6.
     
     If nothing is selected, it will return -1.
     */
    int getLastRowSelected() const;
    
    /** Multiply-selects rows based on the modifier keys.
     
     If no modifier keys are down, this will select the given row and
     deselect any others.
     
     If the ctrl (or command on the Mac) key is down, it'll flip the
     state of the selected row.
     
     If the shift key is down, it'll select up to the given row from the
     last row selected.
     
     @see selectRow
     */
    void selectRowsBasedOnModifierKeys (int rowThatWasClickedOn,
                                        ModifierKeys modifiers,
                                        bool isMouseUpEvent);
    
    
    /** Scrolls the list to a particular position.
     
     The proportion is between 0 and 1.0, so 0 scrolls to the top of the list,
     1.0 scrolls to the bottom.
     
     If the total number of rows all fit onto the screen at once, then this
     method won't do anything.
     
     @see getVerticalPosition
     */
    void setVerticalPosition (double newProportion);
    
    /** Returns the current vertical position as a proportion of the total.
     
     This can be used in conjunction with setVerticalPosition() to save and restore
     the list's position. It returns a value in the range 0 to 1.
     
     @see setVerticalPosition
     */
    double getVerticalPosition() const;
    
    /** Scrolls if necessary to make sure that a particular row is visible. */
    void scrollToEnsureRowIsOnscreen (int row);
    
    /** Returns a pointer to the vertical scrollbar. */
    ScrollBar* getVerticalScrollBar() const noexcept;
    
    /** Returns a pointer to the horizontal scrollbar. */
    ScrollBar* getHorizontalScrollBar() const noexcept;
    
    /** Finds the row index that contains a given x,y position.
     The position is relative to the ListBox's top-left.
     If no row exists at this position, the method will return -1.
     @see getComponentForRowNumber
     */
    int getRowContainingPosition (int x, int y) const noexcept;
    
    /** Finds a row index that would be the most suitable place to insert a new
     item for a given position.
     
     This is useful when the user is e.g. dragging and dropping onto the listbox,
     because it lets you easily choose the best position to insert the item that
     they drop, based on where they drop it.
     
     If the position is out of range, this will return -1. If the position is
     beyond the end of the list, it will return getNumRows() to indicate the end
     of the list.
     
     @see getComponentForRowNumber
     */
    int getInsertionIndexForPosition (int x, int y) const noexcept;
    
    /** Returns the position of one of the rows, relative to the top-left of
     the listbox.
     
     This may be off-screen, and the range of the row number that is passed-in is
     not checked to see if it's a valid row.
     */
    Rectangle<int> getRowPosition (int rowNumber,
                                   bool relativeToComponentTopLeft) const noexcept;
    
    /** Finds the row component for a given row in the list.
     
     The component returned will have been created using createRowComponent().
     
     If the component for this row is off-screen or if the row is out-of-range,
     this will return nullptr.
     
     @see getRowContainingPosition
     */
    Component* getComponentForRowNumber (int rowNumber) const noexcept;
    
    /** Returns the row number that the given component represents.
     If the component isn't one of the list's rows, this will return -1.
     */
    int getRowNumberOfComponent (Component* rowComponent) const noexcept;
    
    /** Returns the width of a row (which may be less than the width of this component
     if there's a scrollbar).
     */
    int getVisibleRowWidth() const noexcept;
    
    
    /** Sets the height of each row in the list.
     The default height is 22 pixels.
     @see getRowHeight
     */
    void setRowHeight (int newHeight);
    
    /** Returns the height of a row in the list.
     @see setRowHeight
     */
    int getRowHeight() const noexcept                   { return rowHeight; }
    
    /** Returns the number of rows actually visible.
     
     This is the number of whole rows which will fit on-screen, so the value might
     be more than the actual number of rows in the list.
     */
    int getNumRowsOnScreen() const noexcept;
    
    
    /** A set of colour IDs to use to change the colour of various aspects of the label.
     
     These constants can be used either via the Component::setColour(), or LookAndFeel::setColour()
     methods.
     
     @see Component::setColour, Component::findColour, LookAndFeel::setColour, LookAndFeel::findColour
     */
    enum ColourIds
    {
        backgroundColourId      = 0x1002800, /**< The background colour to fill the list with.
                                              Make this transparent if you don't want the background to be filled. */
        outlineColourId         = 0x1002810, /**< An optional colour to use to draw a border around the list.
                                              Make this transparent to not have an outline. */
        textColourId            = 0x1002820  /**< The preferred colour to use for drawing text in the listbox. */
    };
    
    /** Sets the thickness of a border that will be drawn around the box.
     
     To set the colour of the outline, use @code setColour (ListBox::outlineColourId, colourXYZ); @endcode
     @see outlineColourId
     */
    void setOutlineThickness (int outlineThickness);
    
    /** Returns the thickness of outline that will be drawn around the listbox.
     @see setOutlineColour
     */
    int getOutlineThickness() const noexcept            { return outlineThickness; }
    
    /** Sets a component that the list should use as a header.
     
     This will position the given component at the top of the list, maintaining the
     height of the component passed-in, but rescaling it horizontally to match the
     width of the items in the listbox.
     
     The component will be deleted when setHeaderComponent() is called with a
     different component, or when the listbox is deleted.
     */
    void setHeaderComponent (Component* newHeaderComponent);
    
    /** Returns whatever header component was set with setHeaderComponent(). */
    Component* getHeaderComponent() const noexcept      { return headerComponent; }
    
    /** Changes the width of the rows in the list.
     
     This can be used to make the list's row components wider than the list itself - the
     width of the rows will be either the width of the list or this value, whichever is
     greater, and if the rows become wider than the list, a horizontal scrollbar will
     appear.
     
     The default value for this is 0, which means that the rows will always
     be the same width as the list.
     */
    void setMinimumContentWidth (int newMinimumWidth);
    
    /** Returns the space currently available for the row items, taking into account
     borders, scrollbars, etc.
     */
    int getVisibleContentWidth() const noexcept;
    
    /** Repaints one of the rows.
     
     This does not invoke updateContent(), it just invokes a straightforward repaint
     for the area covered by this row.
     */
    void repaintRow (int rowNumber) noexcept;
    
    /** This fairly obscure method creates an image that just shows the currently
     selected row components.
     
     It's a handy method for doing drag-and-drop, as it can be passed to the
     DragAndDropContainer for use as the drag image.
     
     Note that it will make the row components temporarily invisible, so if you're
     using custom components this could affect them if they're sensitive to that
     sort of thing.
     
     @see Component::createComponentSnapshot
     */
    virtual Image createSnapshotOfSelectedRows (int& x, int& y);
    
    /** Returns the viewport that this ListBox uses.
     
     You may need to use this to change parameters such as whether scrollbars
     are shown, etc.
     */
    Viewport* getViewport() const noexcept;
    
    
    /** @internal */
    bool keyPressed (const KeyPress&) override;
    /** @internal */
    bool keyStateChanged (bool isKeyDown) override;
    /** @internal */
    void paint (Graphics&) override;
    /** @internal */
    void paintOverChildren (Graphics&) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void visibilityChanged() override;
    /** @internal */
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails&) override;
    /** @internal */
    void mouseUp (const MouseEvent&) override;
    /** @internal */
    void colourChanged() override;
    /** @internal */
    void parentHierarchyChanged() override;
    /** @internal */
    void startDragAndDrop (const MouseEvent&, const var& dragDescription, bool allowDraggingToOtherWindows);
    
private:
    
    JUCE_PUBLIC_IN_DLL_BUILD (class ListViewport)
    JUCE_PUBLIC_IN_DLL_BUILD (class RowComponent)
    friend class ListViewport;
    friend class TableListBox;
    ListBoxModel* model;
    ScopedPointer<ListViewport> viewport;
    ScopedPointer<Component> headerComponent;
    ScopedPointer<MouseListener> mouseMoveSelector;
    int totalItems, rowHeight, minimumRowWidth;
    int outlineThickness;
    int lastRowSelected;
    bool multipleSelection, alwaysFlipSelection, hasDoneInitialUpdate;
    SparseSet<int> selected;
    
    void selectRowInternal (int rowNumber, bool dontScrollToShowThisRow,
                            bool deselectOthersFirst, bool isMouseClick);
    
#if JUCE_CATCH_DEPRECATED_CODE_MISUSE
    // This method's bool parameter has changed: see the new method signature.
    JUCE_DEPRECATED (void setSelectedRows (const SparseSet<int>&, bool));
#endif
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HorizontalListBox)
};

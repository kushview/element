// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/horizontallistbox.hpp"

namespace element {

class HorizontalListBox::RowComponent : public Component,
                                        public TooltipClient
{
public:
    RowComponent (HorizontalListBox& lb)
        : owner (lb), row (-1), selected (false), isDragging (false), selectRowOnMouseUp (false)
    {
    }

    void paint (Graphics& g) override
    {
        if (ListBoxModel* m = owner.getModel())
            m->paintListBoxItem (row, g, getWidth(), getHeight(), selected);
    }

    void update (const int newRow, const bool nowSelected)
    {
        if (row != newRow || selected != nowSelected)
        {
            repaint();
            row = newRow;
            selected = nowSelected;
        }

        if (ListBoxModel* m = owner.getModel())
        {
            setMouseCursor (m->getMouseCursorForRow (row));

            customComponent.reset (m->refreshComponentForRow (newRow, nowSelected, customComponent.release()));

            if (customComponent != nullptr)
            {
                addAndMakeVisible (customComponent.get());
                customComponent->setBounds (getLocalBounds());
            }
        }
    }

    void mouseDown (const MouseEvent& e) override
    {
        isDragging = false;
        selectRowOnMouseUp = false;

        if (isEnabled())
        {
            if (! selected)
            {
                owner.selectRowsBasedOnModifierKeys (row, e.mods, false);

                if (ListBoxModel* m = owner.getModel())
                    m->listBoxItemClicked (row, e);
            }
            else
            {
                selectRowOnMouseUp = true;
            }
        }
    }

    void mouseUp (const MouseEvent& e) override
    {
        if (isEnabled() && selectRowOnMouseUp && ! isDragging)
        {
            owner.selectRowsBasedOnModifierKeys (row, e.mods, true);

            if (ListBoxModel* m = owner.getModel())
                m->listBoxItemClicked (row, e);
        }
    }

    void mouseDoubleClick (const MouseEvent& e) override
    {
        if (ListBoxModel* m = owner.getModel())
            if (isEnabled())
                m->listBoxItemDoubleClicked (row, e);
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (ListBoxModel* m = owner.getModel())
        {
            if (isEnabled() && ! (e.mouseWasClicked() || isDragging))
            {
                const SparseSet<int> selectedRows (owner.getSelectedRows());

                if (selectedRows.size() > 0)
                {
                    const var dragDescription (m->getDragSourceDescription (selectedRows));

                    if (! (dragDescription.isVoid() || (dragDescription.isString() && dragDescription.toString().isEmpty())))
                    {
                        isDragging = true;
                        owner.startDragAndDrop (e, dragDescription, true);
                    }
                }
            }
        }
    }

    void resized() override
    {
        if (customComponent != nullptr)
            customComponent->setBounds (getLocalBounds());
    }

    String getTooltip() override
    {
        if (ListBoxModel* m = owner.getModel())
            return m->getTooltipForRow (row);

        return String();
    }

    std::unique_ptr<Component> customComponent;

private:
    HorizontalListBox& owner;
    int row;
    bool selected, isDragging, selectRowOnMouseUp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RowComponent)
};

class HorizontalListBox::ListViewport : public Viewport
{
public:
    ListViewport (HorizontalListBox& lb)
        : owner (lb)
    {
        setWantsKeyboardFocus (false);

        Component* const content = new Component();
        setViewedComponent (content);
        content->setWantsKeyboardFocus (false);
    }

    RowComponent* getComponentForRow (const int row) const noexcept
    {
        return rows[row % jmax (1, rows.size())];
    }

    RowComponent* getComponentForRowIfOnscreen (const int row) const noexcept
    {
        return (row >= firstIndex && row < firstIndex + rows.size())
                   ? getComponentForRow (row)
                   : nullptr;
    }

    int getRowNumberOfComponent (Component* const rowComponent) const noexcept
    {
        const int index = getViewedComponent()->getIndexOfChildComponent (rowComponent);
        const int num = rows.size();

        for (int i = num; --i >= 0;)
            if (((firstIndex + i) % jmax (1, num)) == index)
                return firstIndex + i;

        return -1;
    }

    void visibleAreaChanged (const Rectangle<int>&) override
    {
        updateVisibleArea (true);

        if (ListBoxModel* m = owner.getModel())
            m->listWasScrolled();
    }

    void updateVisibleArea (const bool makeSureItUpdatesContent)
    {
        hasUpdated = false;

        Component& content = *getViewedComponent();
        int newX = content.getX();
        const int newY = content.getY();
        const int newH = jmax (owner.minimumRowWidth, getMaximumVisibleHeight());
        const int newW = owner.totalItems * owner.getRowHeight();

        if (newX + newW < getMaximumVisibleWidth() && newW > getMaximumVisibleWidth())
            newX = getMaximumVisibleWidth() - newW;

        content.setBounds (newX, newY, newW, newH);

        if (makeSureItUpdatesContent && ! hasUpdated)
            updateContents();
    }

    void updateContents()
    {
        hasUpdated = true;
        const int rowH = owner.getRowHeight();
        Component& content = *getViewedComponent();

        if (rowH > 0)
        {
            const int x = getViewPositionX();
            const int h = content.getHeight();

            const int numNeeded = 2 + getMaximumVisibleWidth() / rowH;
            rows.removeRange (numNeeded, rows.size());

            while (numNeeded > rows.size())
            {
                RowComponent* newRow = new RowComponent (owner);
                rows.add (newRow);
                content.addAndMakeVisible (newRow);
            }

            firstIndex = x / rowH;
            firstWholeIndex = (x + rowH - 1) / rowH;
            lastWholeIndex = (x + getMaximumVisibleWidth() - 1) / rowH;

            for (int i = 0; i < numNeeded; ++i)
            {
                const int row = i + firstIndex;

                if (RowComponent* const rowComp = getComponentForRow (row))
                {
                    rowComp->setBounds (row * rowH, 0, rowH, h);
                    rowComp->update (row, owner.isRowSelected (row));
                }
            }
        }

        if (owner.headerComponent != nullptr)
            owner.headerComponent->setBounds (owner.outlineThickness + content.getX(),
                                              owner.outlineThickness,
                                              jmax (owner.getWidth() - owner.outlineThickness * 2,
                                                    content.getWidth()),
                                              owner.headerComponent->getHeight());
    }

    void selectRow (const int row, const int rowH, const bool dontScroll, const int lastSelectedRow, const int totalRows, const bool isMouseClick)
    {
        hasUpdated = false;

        if (row < firstWholeIndex && ! dontScroll)
        {
            setViewPosition (getViewPositionX(), row * rowH);
        }
        else if (row >= lastWholeIndex && ! dontScroll)
        {
            const int rowsOnScreen = lastWholeIndex - firstWholeIndex;

            if (row >= lastSelectedRow + rowsOnScreen
                && rowsOnScreen < totalRows - 1
                && ! isMouseClick)
            {
                setViewPosition (getViewPositionX(),
                                 jlimit (0, jmax (0, totalRows - rowsOnScreen), row) * rowH);
            }
            else
            {
                setViewPosition (getViewPositionX(),
                                 jmax (0, (row + 1) * rowH - getMaximumVisibleHeight()));
            }
        }

        if (! hasUpdated)
            updateContents();
    }

    void scrollToEnsureRowIsOnscreen (const int row, const int rowH)
    {
        if (row < firstWholeIndex)
        {
            setViewPosition (getViewPositionX(), row * rowH);
        }
        else if (row >= lastWholeIndex)
        {
            setViewPosition (getViewPositionX(),
                             jmax (0, (row + 1) * rowH - getMaximumVisibleHeight()));
        }
    }

    void paint (Graphics& g) override
    {
        if (isOpaque())
            g.fillAll (owner.findColour (HorizontalListBox::backgroundColourId));
    }

    bool keyPressed (const KeyPress& key) override
    {
        if (Viewport::respondsToKey (key))
        {
            const int allowableMods = owner.multipleSelection ? ModifierKeys::shiftModifier : 0;

            if ((key.getModifiers().getRawFlags() & ~allowableMods) == 0)
            {
                // we want to avoid these keypresses going to the viewport, and instead allow
                // them to pass up to our listbox..
                return false;
            }
        }

        return Viewport::keyPressed (key);
    }

private:
    HorizontalListBox& owner;
    OwnedArray<RowComponent> rows;
    int firstIndex, firstWholeIndex, lastWholeIndex;
    bool hasUpdated;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ListViewport)
};

//==============================================================================
class ListBoxMouseMoveSelector : public MouseListener
{
public:
    ListBoxMouseMoveSelector (HorizontalListBox& lb) : owner (lb)
    {
        owner.addMouseListener (this, true);
    }

    void mouseMove (const MouseEvent& e) override
    {
        const MouseEvent e2 (e.getEventRelativeTo (&owner));
        owner.selectRow (owner.getRowContainingPosition (e2.x, e2.y), true);
    }

    void mouseExit (const MouseEvent& e) override
    {
        mouseMove (e);
    }

private:
    HorizontalListBox& owner;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ListBoxMouseMoveSelector)
};

//==============================================================================
HorizontalListBox::HorizontalListBox (const String& name, ListBoxModel* const m)
    : Component (name),
      model (m),
      totalItems (0),
      rowHeight (22),
      minimumRowWidth (0),
      outlineThickness (0),
      lastRowSelected (-1),
      multipleSelection (false),
      alwaysFlipSelection (false),
      hasDoneInitialUpdate (false)
{
    viewport = std::make_unique<ListViewport> (*this);
    addAndMakeVisible (viewport.get());

    HorizontalListBox::setWantsKeyboardFocus (true);
    HorizontalListBox::colourChanged();
}

HorizontalListBox::~HorizontalListBox()
{
    headerComponent = nullptr;
    viewport = nullptr;
}

void HorizontalListBox::setModel (ListBoxModel* const newModel)
{
    if (model != newModel)
    {
        model = newModel;
        repaint();
        updateContent();
    }
}

void HorizontalListBox::setMultipleSelectionEnabled (bool b) noexcept
{
    multipleSelection = b;
}

void HorizontalListBox::setClickingTogglesRowSelection (bool b) noexcept
{
    alwaysFlipSelection = b;
}

void HorizontalListBox::setMouseMoveSelectsRows (bool b)
{
    if (b)
    {
        if (mouseMoveSelector == nullptr)
            mouseMoveSelector = std::make_unique<ListBoxMouseMoveSelector> (*this);
    }
    else
    {
        mouseMoveSelector = nullptr;
    }
}

//==============================================================================
void HorizontalListBox::paint (Graphics& g)
{
    if (! hasDoneInitialUpdate)
        updateContent();

    g.fillAll (findColour (backgroundColourId));
}

void HorizontalListBox::paintOverChildren (Graphics& g)
{
    if (outlineThickness > 0)
    {
        g.setColour (findColour (outlineColourId));
        g.drawRect (getLocalBounds(), outlineThickness);
    }
}

void HorizontalListBox::resized()
{
    viewport->setBoundsInset (BorderSize<int> (outlineThickness + (headerComponent != nullptr ? headerComponent->getHeight() : 0),
                                               outlineThickness,
                                               outlineThickness,
                                               outlineThickness));

    viewport->setSingleStepSizes (20, getRowHeight());

    viewport->updateVisibleArea (false);
}

void HorizontalListBox::visibilityChanged()
{
    viewport->updateVisibleArea (true);
}

Viewport* HorizontalListBox::getViewport() const noexcept
{
    return viewport.get();
}

//==============================================================================
void HorizontalListBox::updateContent()
{
    hasDoneInitialUpdate = true;
    totalItems = (model != nullptr) ? model->getNumRows() : 0;

    bool selectionChanged = false;

    if (selected.size() > 0 && selected[selected.size() - 1] >= totalItems)
    {
        selected.removeRange (Range<int> (totalItems, std::numeric_limits<int>::max()));
        lastRowSelected = getSelectedRow (0);
        selectionChanged = true;
    }

    viewport->updateVisibleArea (isVisible());
    viewport->resized();

    if (selectionChanged && model != nullptr)
        model->selectedRowsChanged (lastRowSelected);
}

//==============================================================================
void HorizontalListBox::selectRow (int row, bool dontScroll, bool deselectOthersFirst)
{
    selectRowInternal (row, dontScroll, deselectOthersFirst, false);
}

void HorizontalListBox::selectRowInternal (const int row,
                                           bool dontScroll,
                                           bool deselectOthersFirst,
                                           bool isMouseClick)
{
    if (! multipleSelection)
        deselectOthersFirst = true;

    if ((! isRowSelected (row))
        || (deselectOthersFirst && getNumSelectedRows() > 1))
    {
        if (isPositiveAndBelow (row, totalItems))
        {
            if (deselectOthersFirst)
                selected.clear();

            selected.addRange (Range<int> (row, row + 1));

            if (getHeight() == 0 || getWidth() == 0)
                dontScroll = true;

            viewport->selectRow (row, getRowHeight(), dontScroll, lastRowSelected, totalItems, isMouseClick);

            lastRowSelected = row;
            model->selectedRowsChanged (row);
        }
        else
        {
            if (deselectOthersFirst)
                deselectAllRows();
        }
    }
}

void HorizontalListBox::deselectRow (const int row)
{
    if (selected.contains (row))
    {
        selected.removeRange (Range<int> (row, row + 1));

        if (row == lastRowSelected)
            lastRowSelected = getSelectedRow (0);

        viewport->updateContents();
        model->selectedRowsChanged (lastRowSelected);
    }
}

void HorizontalListBox::setSelectedRows (const SparseSet<int>& setOfRowsToBeSelected,
                                         const NotificationType sendNotificationEventToModel)
{
    selected = setOfRowsToBeSelected;
    selected.removeRange (Range<int> (totalItems, std::numeric_limits<int>::max()));

    if (! isRowSelected (lastRowSelected))
        lastRowSelected = getSelectedRow (0);

    viewport->updateContents();

    if (model != nullptr && sendNotificationEventToModel == sendNotification)
        model->selectedRowsChanged (lastRowSelected);
}

SparseSet<int> HorizontalListBox::getSelectedRows() const
{
    return selected;
}

void HorizontalListBox::selectRangeOfRows (int firstRow, int lastRow)
{
    if (multipleSelection && (firstRow != lastRow))
    {
        const int numRows = totalItems - 1;
        firstRow = jlimit (0, jmax (0, numRows), firstRow);
        lastRow = jlimit (0, jmax (0, numRows), lastRow);

        selected.addRange (Range<int> (jmin (firstRow, lastRow),
                                       jmax (firstRow, lastRow) + 1));

        selected.removeRange (Range<int> (lastRow, lastRow + 1));
    }

    selectRowInternal (lastRow, false, false, true);
}

void HorizontalListBox::flipRowSelection (const int row)
{
    if (isRowSelected (row))
        deselectRow (row);
    else
        selectRowInternal (row, false, false, true);
}

void HorizontalListBox::deselectAllRows()
{
    if (! selected.isEmpty())
    {
        selected.clear();
        lastRowSelected = -1;

        viewport->updateContents();

        if (model != nullptr)
            model->selectedRowsChanged (lastRowSelected);
    }
}

void HorizontalListBox::selectRowsBasedOnModifierKeys (const int row,
                                                       ModifierKeys mods,
                                                       const bool isMouseUpEvent)
{
    if (multipleSelection && (mods.isCommandDown() || alwaysFlipSelection))
    {
        flipRowSelection (row);
    }
    else if (multipleSelection && mods.isShiftDown() && lastRowSelected >= 0)
    {
        selectRangeOfRows (lastRowSelected, row);
    }
    else if ((! mods.isPopupMenu()) || ! isRowSelected (row))
    {
        selectRowInternal (row, false, ! (multipleSelection && (! isMouseUpEvent) && isRowSelected (row)), true);
    }
}

int HorizontalListBox::getNumSelectedRows() const
{
    return selected.size();
}

int HorizontalListBox::getSelectedRow (const int index) const
{
    return (isPositiveAndBelow (index, selected.size()))
               ? selected[index]
               : -1;
}

bool HorizontalListBox::isRowSelected (const int row) const
{
    return selected.contains (row);
}

int HorizontalListBox::getLastRowSelected() const
{
    return isRowSelected (lastRowSelected) ? lastRowSelected : -1;
}

//==============================================================================
int HorizontalListBox::getRowContainingPosition (const int x, const int y) const noexcept
{
    if (isPositiveAndBelow (x, getWidth()))
    {
        const int row = (viewport->getViewPositionY() + y - viewport->getY()) / rowHeight;

        if (isPositiveAndBelow (row, totalItems))
            return row;
    }

    return -1;
}

int HorizontalListBox::getInsertionIndexForPosition (const int x, const int y) const noexcept
{
    if (isPositiveAndBelow (x, getWidth()))
    {
        const int row = (viewport->getViewPositionY() + y + rowHeight / 2 - viewport->getY()) / rowHeight;
        return jlimit (0, totalItems, row);
    }

    return -1;
}

Component* HorizontalListBox::getComponentForRowNumber (const int row) const noexcept
{
    if (RowComponent* const listRowComp = viewport->getComponentForRowIfOnscreen (row))
        return static_cast<Component*> (listRowComp->customComponent.get());

    return nullptr;
}

int HorizontalListBox::getRowNumberOfComponent (Component* const rowComponent) const noexcept
{
    return viewport->getRowNumberOfComponent (rowComponent);
}

Rectangle<int> HorizontalListBox::getRowPosition (const int rowNumber,
                                                  const bool relativeToComponentTopLeft) const noexcept
{
    int x = viewport->getX() + rowHeight * rowNumber;

    if (relativeToComponentTopLeft)
        x -= viewport->getViewPositionX();

    return { x, viewport->getY(), rowHeight, viewport->getViewedComponent()->getHeight() };
}

void HorizontalListBox::setVerticalPosition (const double proportion)
{
    const int offscreen = viewport->getViewedComponent()->getHeight() - viewport->getHeight();

    viewport->setViewPosition (viewport->getViewPositionX(),
                               jmax (0, roundToInt (proportion * offscreen)));
}

double HorizontalListBox::getVerticalPosition() const
{
    const int offscreen = viewport->getViewedComponent()->getHeight() - viewport->getHeight();

    return (offscreen > 0) ? viewport->getViewPositionY() / (double) offscreen
                           : 0;
}

int HorizontalListBox::getVisibleRowWidth() const noexcept
{
    return viewport->getViewWidth();
}

void HorizontalListBox::scrollToEnsureRowIsOnscreen (const int row)
{
    viewport->scrollToEnsureRowIsOnscreen (row, getRowHeight());
}

//==============================================================================
bool HorizontalListBox::keyPressed (const KeyPress& key)
{
    const int numVisibleRows = viewport->getHeight() / getRowHeight();

    const bool multiple = multipleSelection
                          && lastRowSelected >= 0
                          && key.getModifiers().isShiftDown();

    if (key.isKeyCode (KeyPress::upKey))
    {
        if (multiple)
            selectRangeOfRows (lastRowSelected, lastRowSelected - 1);
        else
            selectRow (jmax (0, lastRowSelected - 1));
    }
    else if (key.isKeyCode (KeyPress::downKey))
    {
        if (multiple)
            selectRangeOfRows (lastRowSelected, lastRowSelected + 1);
        else
            selectRow (jmin (totalItems - 1, jmax (0, lastRowSelected) + 1));
    }
    else if (key.isKeyCode (KeyPress::pageUpKey))
    {
        if (multiple)
            selectRangeOfRows (lastRowSelected, lastRowSelected - numVisibleRows);
        else
            selectRow (jmax (0, jmax (0, lastRowSelected) - numVisibleRows));
    }
    else if (key.isKeyCode (KeyPress::pageDownKey))
    {
        if (multiple)
            selectRangeOfRows (lastRowSelected, lastRowSelected + numVisibleRows);
        else
            selectRow (jmin (totalItems - 1, jmax (0, lastRowSelected) + numVisibleRows));
    }
    else if (key.isKeyCode (KeyPress::homeKey))
    {
        if (multiple)
            selectRangeOfRows (lastRowSelected, 0);
        else
            selectRow (0);
    }
    else if (key.isKeyCode (KeyPress::endKey))
    {
        if (multiple)
            selectRangeOfRows (lastRowSelected, totalItems - 1);
        else
            selectRow (totalItems - 1);
    }
    else if (key.isKeyCode (KeyPress::returnKey) && isRowSelected (lastRowSelected))
    {
        if (model != nullptr)
            model->returnKeyPressed (lastRowSelected);
    }
    else if ((key.isKeyCode (KeyPress::deleteKey) || key.isKeyCode (KeyPress::backspaceKey))
             && isRowSelected (lastRowSelected))
    {
        if (model != nullptr)
            model->deleteKeyPressed (lastRowSelected);
    }
    else if (multipleSelection && key == KeyPress ('a', ModifierKeys::commandModifier, 0))
    {
        selectRangeOfRows (0, std::numeric_limits<int>::max());
    }
    else
    {
        return false;
    }

    return true;
}

bool HorizontalListBox::keyStateChanged (const bool isKeyDown)
{
    return isKeyDown
           && (KeyPress::isKeyCurrentlyDown (KeyPress::upKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::pageUpKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::downKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::pageDownKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::homeKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::endKey)
               || KeyPress::isKeyCurrentlyDown (KeyPress::returnKey));
}

void HorizontalListBox::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel)
{
    bool eventWasUsed = false;

    if (wheel.deltaX != 0 && viewport->getHorizontalScrollBar().isVisible())
    {
        eventWasUsed = true;
        viewport->getHorizontalScrollBar().mouseWheelMove (e, wheel);
    }

    if (wheel.deltaY != 0 && viewport->getVerticalScrollBar().isVisible())
    {
        eventWasUsed = true;
        viewport->getVerticalScrollBar().mouseWheelMove (e, wheel);
    }

    if (! eventWasUsed)
        Component::mouseWheelMove (e, wheel);
}

void HorizontalListBox::mouseUp (const MouseEvent& e)
{
    if (e.mouseWasClicked() && model != nullptr)
        model->backgroundClicked (e);
}

//==============================================================================
void HorizontalListBox::setRowHeight (const int newHeight)
{
    rowHeight = jmax (1, newHeight);
    viewport->setSingleStepSizes (20, rowHeight);
    updateContent();
}

int HorizontalListBox::getNumRowsOnScreen() const noexcept
{
    return viewport->getMaximumVisibleHeight() / rowHeight;
}

void HorizontalListBox::setMinimumContentWidth (const int newMinimumWidth)
{
    minimumRowWidth = newMinimumWidth;
    updateContent();
}

int HorizontalListBox::getVisibleContentWidth() const noexcept
{
    return viewport->getMaximumVisibleWidth();
}

void HorizontalListBox::setScrollBarsShown (bool vertical, bool horizontal)
{
    viewport->setScrollBarsShown (vertical, horizontal, ! vertical, ! horizontal);
    resized();
}

void HorizontalListBox::setScrollBarPosition (bool right, bool bottom)
{
    viewport->setScrollBarPosition (right, bottom);
}

ScrollBar* HorizontalListBox::getVerticalScrollBar() const noexcept
{
    return &viewport->getVerticalScrollBar();
}

ScrollBar* HorizontalListBox::getHorizontalScrollBar() const noexcept
{
    return &viewport->getHorizontalScrollBar();
}

void HorizontalListBox::colourChanged()
{
    setOpaque (findColour (backgroundColourId).isOpaque());
    viewport->setOpaque (isOpaque());
    repaint();
}

void HorizontalListBox::parentHierarchyChanged()
{
    colourChanged();
}

void HorizontalListBox::setOutlineThickness (const int newThickness)
{
    outlineThickness = newThickness;
    resized();
}

void HorizontalListBox::setHeaderComponent (Component* const newHeaderComponent)
{
    if (headerComponent.get() != newHeaderComponent)
    {
        headerComponent.reset (newHeaderComponent);

        addAndMakeVisible (newHeaderComponent);
        HorizontalListBox::resized();
    }
}

void HorizontalListBox::repaintRow (const int rowNumber) noexcept
{
    repaint (getRowPosition (rowNumber, true));
}

Image HorizontalListBox::createSnapshotOfSelectedRows (int& imageX, int& imageY)
{
    Rectangle<int> imageArea;
    const int firstRow = getRowContainingPosition (0, viewport->getY());

    for (int i = getNumRowsOnScreen() + 2; --i >= 0;)
    {
        Component* rowComp = viewport->getComponentForRowIfOnscreen (firstRow + i);

        if (rowComp != nullptr && isRowSelected (firstRow + i))
        {
            const Point<int> pos (getLocalPoint (rowComp, Point<int>()));
            const Rectangle<int> rowRect (pos.getX(), pos.getY(), rowComp->getWidth(), rowComp->getHeight());
            imageArea = imageArea.getUnion (rowRect);
        }
    }

    imageArea = imageArea.getIntersection (getLocalBounds());
    imageX = imageArea.getX();
    imageY = imageArea.getY();
    Image snapshot (Image::ARGB, imageArea.getWidth(), imageArea.getHeight(), true);

    for (int i = getNumRowsOnScreen() + 2; --i >= 0;)
    {
        Component* rowComp = viewport->getComponentForRowIfOnscreen (firstRow + i);

        if (rowComp != nullptr && isRowSelected (firstRow + i))
        {
            Graphics g (snapshot);
            g.setOrigin (getLocalPoint (rowComp, Point<int>()) - imageArea.getPosition());

            if (g.reduceClipRegion (rowComp->getLocalBounds()))
            {
                g.beginTransparencyLayer (0.6f);
                rowComp->paintEntireComponent (g, false);
                g.endTransparencyLayer();
            }
        }
    }

    return snapshot;
}

void HorizontalListBox::startDragAndDrop (const MouseEvent& e, const var& dragDescription, bool allowDraggingToOtherWindows)
{
    if (DragAndDropContainer* const dragContainer = DragAndDropContainer::findParentDragContainerFor (this))
    {
        int x, y;
        Image dragImage (createSnapshotOfSelectedRows (x, y));

        MouseEvent e2 (e.getEventRelativeTo (this));
        const Point<int> p (x - e2.x, y - e2.y);
        dragContainer->startDragging (dragDescription,
                                      this,
                                      ScaledImage (dragImage),
                                      allowDraggingToOtherWindows,
                                      &p);
    }
    else
    {
        // to be able to do a drag-and-drop operation, the listbox needs to
        // be inside a component which is also a DragAndDropContainer.
        jassertfalse;
    }
}

} // namespace element

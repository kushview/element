/*
    TreeviewBase.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

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

#include "gui/TreeviewBase.h"

namespace Element {
    
void TreePanelBase::setRoot (TreeItemBase* root)
{
    tree.setRootItem (nullptr);
    rootItem = root;
    if (root != nullptr) {
        tree.setRootItem (root);
        tree.getRootItem()->setOpen (true);
    }

#if 0
    if (project != nullptr)
    {
        const ScopedPointer<XmlElement> treeOpenness (project->getStoredProperties()
                                                          .getXmlValue (opennessStateKey));
        if (treeOpenness != nullptr)
        {
            tree.restoreOpennessState (*treeOpenness, true);

            for (int i = tree.getNumSelectedItems(); --i >= 0;)
                if (JucerTreeViewBase* item = dynamic_cast<JucerTreeViewBase*> (tree.getSelectedItem (i)))
                    item->cancelDelayedSelectionTimer();
        }
    }
#endif
}

void TreePanelBase::saveOpenness()
{
#if 0
    if (project != nullptr)
    {
        const ScopedPointer<XmlElement> opennessState (tree.getOpennessState (true));

        if (opennessState != nullptr)
            project->getStoredProperties().setValue (opennessStateKey, opennessState);
        else
            project->getStoredProperties().removeValue (opennessStateKey);
    }
#endif
}

//==============================================================================
TreeItemBase::TreeItemBase()  : textX (0)
{
    setLinesDrawnForSubItems (false);
}

TreeItemBase::~TreeItemBase()
{
    masterReference.clear();
}

void TreeItemBase::refreshSubItems()
{
    WholeTreeOpennessRestorer wtor (*this);
    clearSubItems();
    addSubItems();
}

Font
TreeItemBase::getFont() const
{
    return Font (getItemHeight() * 0.7f);
}

void
TreeItemBase::paintItem (Graphics& g, int /*width*/, int /*height*/)
{
    if (isSelected())
        g.fillAll (Colours::aqua);
}

float TreeItemBase::getIconSize() const
{
    return jmin (getItemHeight() - 4.0f, 18.0f);
}

void TreeItemBase::paintOpenCloseButton (Graphics& g, const Rectangle<float>& area,
                                         Colour /* backgroundColour */, bool /* isMouseOver */)
{
    Path p;

    const float width = area.getWidth();
    const float height = area.getHeight();

    if (isOpen())
        p.addTriangle (width * 0.2f,  height * 0.25f, width * 0.8f, height * 0.25f, width * 0.5f, height * 0.75f);
    else
        p.addTriangle (width * 0.25f, height * 0.25f, width * 0.8f, height * 0.5f,  width * 0.25f, height * 0.75f);

    g.setColour (getOwnerView()->findColour (mainBackgroundColourId).contrasting (0.3f));
    g.fillPath (p);
}

Colour
TreeItemBase::getBackgroundColour() const
{
#if 0
    Colour background (getOwnerView()->findColour (gui::mainBackgroundColourId));

    if (isSelected())
        background = background.overlaidWith (getOwnerView()->findColour (gui::treeviewHighlightColourId));
#else
    Colour background (Colours::transparentBlack);

    if (isSelected())
        background = background.overlaidWith (Colours::aqua.darker());

#endif
    return background;
}

Colour TreeItemBase::getContrastingColour (float contrast) const
{
    return getBackgroundColour().contrasting (contrast);
}

Colour TreeItemBase::getContrastingColour (const Colour& target, float minContrast) const
{
    return getBackgroundColour().contrasting (target, minContrast);
}

void TreeItemBase::paintContent (Graphics& g, const Rectangle<int>& area)
{
    g.setFont (getFont());
    g.setColour (isMissing() ? getContrastingColour (Colours::red, 0.8f)
                             : getContrastingColour (0.8f));

    g.drawFittedText (getDisplayName(), area, Justification::centredLeft, 1, 0.8f);
}

Component* TreeItemBase::createItemComponent()
{
    return new TreeItemComponent (*this);
}

//==============================================================================
class RenameTreeItemCallback  : public ModalComponentManager::Callback,
                                public TextEditorListener
{
public:
    RenameTreeItemCallback (TreeItemBase& ti, Component& parent, const Rectangle<int>& bounds)
        : item (ti)
    {
        ed.setMultiLine (false, false);
        ed.setPopupMenuEnabled (false);
        ed.setSelectAllWhenFocused (true);
        ed.setFont (item.getFont());
        ed.addListener (this);
        ed.setText (item.getRenamingName());
        ed.setBounds (bounds);

        parent.addAndMakeVisible (&ed);
        ed.enterModalState (true, this);
    }

    void modalStateFinished (int resultCode) override
    {
        if (resultCode != 0)
            item.setName (ed.getText());
    }

    void textEditorTextChanged (TextEditor&) override               {}
    void textEditorReturnKeyPressed (TextEditor& editor) override    { editor.exitModalState (1); }
    void textEditorEscapeKeyPressed (TextEditor& editor) override    { editor.exitModalState (0); }
    void textEditorFocusLost (TextEditor& editor) override           { editor.exitModalState (0); }

private:
    struct RenameEditor   : public TextEditor
    {
        void inputAttemptWhenModal() override   { exitModalState (0); }
    };

    RenameEditor ed;
    TreeItemBase& item;

    JUCE_DECLARE_NON_COPYABLE (RenameTreeItemCallback)
};

void TreeItemBase::showRenameBox()
{
    Rectangle<int> r (getItemPosition (true));
    r.setLeft (r.getX() + textX);
    r.setHeight (getItemHeight());

    new RenameTreeItemCallback (*this, *getOwnerView(), r);
}

void TreeItemBase::itemClicked (const MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        if (getOwnerView()->getNumSelectedItems() > 1)
            showMultiSelectionPopupMenu();
        else
            showPopupMenu();
    }
}

void TreeItemBase::deleteItem()    {}
void TreeItemBase::deleteAllSelectedItems() {}
void TreeItemBase::showDocument()  {}
void TreeItemBase::showPopupMenu() {}
void TreeItemBase::showMultiSelectionPopupMenu() {}

static void treeViewMenuItemChosen (int resultCode, WeakReference<TreeItemBase> item)
{
    if (item != nullptr)
        item->handlePopupMenuResult (resultCode);
}

void TreeItemBase::launchPopupMenu (PopupMenu& m)
{
    m.showMenuAsync (PopupMenu::Options(),
                     ModalCallbackFunction::create (treeViewMenuItemChosen, WeakReference<TreeItemBase> (this)));
}

void TreeItemBase::handlePopupMenuResult (int)
{
}

//==============================================================================
class TreeItemBase::ItemSelectionTimer  : public Timer
{
public:
    ItemSelectionTimer (TreeItemBase& tvb)  : owner (tvb) {}

    void timerCallback() override    { owner.invokeShowDocument(); }

private:
    TreeItemBase& owner;
    JUCE_DECLARE_NON_COPYABLE (ItemSelectionTimer)
};

void TreeItemBase::itemSelectionChanged (bool isNowSelected)
{
    if (isNowSelected)
    {
        delayedSelectionTimer = new ItemSelectionTimer (*this);
        delayedSelectionTimer->startTimer (getMillisecsAllowedForDragGesture());
    }
    else
    {
        cancelDelayedSelectionTimer();
    }
}

void TreeItemBase::invokeShowDocument()
{
    cancelDelayedSelectionTimer();
    showDocument();
}

void TreeItemBase::itemDoubleClicked (const MouseEvent&)
{
    invokeShowDocument();
}

void TreeItemBase::cancelDelayedSelectionTimer()
{
    delayedSelectionTimer = nullptr;
}

}

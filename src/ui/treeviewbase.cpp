// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/treeviewbase.hpp"
#include "ui/viewhelpers.hpp"

namespace element {

TreePanelBase::TreePanelBase (const String& treeviewID)
    : opennessStateKey (treeviewID)
{
    addAndMakeVisible (&tree);
    tree.setRootItemVisible (false);
    tree.setDefaultOpenness (true);
    tree.setColour (TreeView::backgroundColourId, Colors::backgroundColor);
    tree.setIndentSize (16);
    tree.setOpenCloseButtonsVisible (true);
    tree.getViewport()->setScrollBarThickness (12);
}

TreePanelBase::~TreePanelBase()
{
    tree.setRootItem (nullptr);
}

void TreePanelBase::setRoot (TreeItemBase* root)
{
    tree.setRootItem (nullptr);
    rootItem.reset (root);
    if (rootItem != nullptr)
    {
        tree.setRootItem (rootItem.get());
        rootItem->setOpen (true);
    }
}

void TreePanelBase::saveOpenness()
{
#if 0
    if (project != nullptr)
    {
        const std::unique_ptr<XmlElement> opennessState (tree.getOpennessState (true));

        if (opennessState != nullptr)
            project->getStoredProperties().setValue (opennessStateKey, opennessState);
        else
            project->getStoredProperties().removeValue (opennessStateKey);
    }
#endif
}

TreeItemBase::TreeItemBase() : textX (0)
{
    setLinesDrawnForSubItems (false);
}

TreeItemBase::~TreeItemBase()
{
    masterReference.clear();
}

void TreeItemBase::refreshSubItems()
{
    // TODO: sub classes MUST provide the unique name
    WholeTreeOpennessRestorer wtor (*this);
    clearSubItems();
    addSubItems();
}

Font TreeItemBase::getFont() const
{
    return Font (FontOptions (getItemHeight() * 0.7f));
}

void TreeItemBase::paintItem (Graphics& g, int w, int h)
{
    if (isSelected())
    {
        ViewHelpers::drawBasicTextRow (String(), g, w, w, true);
    }
}

float TreeItemBase::getIconSize() const
{
    return jmin (getItemHeight() - 4.0f, 10.0f);
}

#if 0
void TreeItemBase::paintOpenCloseButton (Graphics& g, const Rectangle<float>& r, Colour c, bool o)
{
    getOwnerView()->getLookAndFeel().drawTreeviewPlusMinusBox (g, r, c, o, false);
    return;
#if 0
    Path p;

    const float width  = area.getWidth();
    const float height = area.getHeight();

    if (isOpen())
        p.addTriangle (width * 0.2f,  height * 0.25f, width * 0.8f, height * 0.25f, width * 0.5f, height * 0.75f);
    else
        p.addTriangle (width * 0.25f, height * 0.25f, width * 0.8f, height * 0.5f,  width * 0.25f, height * 0.75f);

    g.setColour (Colours::white);
    g.fillPath (p);
#endif
}
#endif

Colour TreeItemBase::getBackgroundColour() const
{
#if 0
    Colour background (Colors::backgroundColor);

    if (isSelected())
        background = background.overlaidWith (Colors::elemental.darker(0.600006f));
#else
    const Colour background (0x000000);
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
                             : Colors::textColor);

    g.drawFittedText (getDisplayName(), area, Justification::centredLeft, 1, 0.8f);
}

std::unique_ptr<Component> TreeItemBase::createItemComponent()
{
    return std::make_unique<TreeItemComponent> (*this);
}

class RenameTreeItemCallback : public ModalComponentManager::Callback,
                               public TextEditor::Listener
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

    void textEditorTextChanged (TextEditor&) override {}
    void textEditorReturnKeyPressed (TextEditor& editor) override { editor.exitModalState (1); }
    void textEditorEscapeKeyPressed (TextEditor& editor) override { editor.exitModalState (0); }
    void textEditorFocusLost (TextEditor& editor) override { editor.exitModalState (0); }

private:
    struct RenameEditor : public TextEditor
    {
        void inputAttemptWhenModal() override { exitModalState (0); }
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

void TreeItemBase::deleteItem() {}
void TreeItemBase::deleteAllSelectedItems() {}
void TreeItemBase::showDocument() {}
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

class TreeItemBase::ItemSelectionTimer : public Timer
{
public:
    ItemSelectionTimer (TreeItemBase& tvb) : owner (tvb) {}
    void timerCallback() override { owner.invokeShowDocument(); }

private:
    TreeItemBase& owner;
    JUCE_DECLARE_NON_COPYABLE (ItemSelectionTimer)
};

void TreeItemBase::itemSelectionChanged (bool isNowSelected)
{
    if (isNowSelected)
    {
        delayedSelectionTimer = std::make_unique<ItemSelectionTimer> (*this);
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

} // namespace element

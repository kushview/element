// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "ui/icons.hpp"
#include <element/ui/style.hpp>

namespace element {

class TreeItemBase : public TreeViewItem
{
public:
    TreeItemBase();
    ~TreeItemBase();

    void refreshSubItems();

    virtual Font getFont() const;
    virtual String getRenamingName() const = 0;
    virtual String getDisplayName() const = 0;
    virtual void setName (const String& newName) = 0;
    virtual bool isMissing() = 0;
    virtual Icon getIcon() const = 0;
    virtual float getIconSize() const;
    virtual bool isIconCrossedOut() const { return false; }
    virtual void paintContent (Graphics& g, const Rectangle<int>& area);
    virtual int getMillisecsAllowedForDragGesture() { return 120; }
    virtual File getDraggableFile() const { return File(); }

    virtual void deleteItem();
    virtual void deleteAllSelectedItems();
    virtual void showDocument();
    virtual void showMultiSelectionPopupMenu();
    virtual void showRenameBox();

    void launchPopupMenu (PopupMenu&); // runs asynchronously, and produces a callback to handlePopupMenuResult().
    virtual void showPopupMenu();
    virtual void handlePopupMenuResult (int resultCode);

    int getItemWidth() const override { return -1; }
    int getItemHeight() const override { return 20; }

    std::unique_ptr<Component> createItemComponent() override;

    void itemClicked (const MouseEvent& e) override;
    void itemDoubleClicked (const MouseEvent&) override;

    void itemSelectionChanged (bool isNowSelected) override;
    void paintItem (Graphics& g, int width, int height) override;
#if 0
    void paintOpenCloseButton (Graphics&, const Rectangle<float>& area, 
                               Colour backgroundColour, bool isMouseOver) override;
#endif

    //=========================================================================
    struct WholeTreeOpennessRestorer : public OpennessRestorer
    {
        WholeTreeOpennessRestorer (TreeViewItem& item)
            : OpennessRestorer (getTopLevelItem (item)) {}

    private:
        static TreeViewItem& getTopLevelItem (TreeViewItem& item)
        {
            if (TreeViewItem* const p = item.getParentItem())
                return getTopLevelItem (*p);

            return item;
        }
    };

    //=========================================================================
    int textX;

protected:
    void cancelDelayedSelectionTimer();

    template <class ParentType>
    inline ParentType* findParent() const
    {
        for (Component* c = getOwnerView(); c != nullptr; c = c->getParentComponent())
            if (ParentType* pcc = dynamic_cast<ParentType*> (c))
                return pcc;

        return nullptr;
    }
    virtual void addSubItems() {}

    Colour getBackgroundColour() const;
    Colour getContrastingColour (float contrast) const;
    Colour getContrastingColour (const Colour& targetColour, float minContrast) const;

private:
    class ItemSelectionTimer;
    friend class ItemSelectionTimer;
    std::unique_ptr<Timer> delayedSelectionTimer;
    WeakReference<TreeItemBase>::Master masterReference;
    friend class WeakReference<TreeItemBase>;

    void invokeShowDocument();
};

class TreePanelBase : public Component
{
public:
    explicit TreePanelBase (const String& treeviewID = "treePanelBase");

    ~TreePanelBase();

    void setRoot (TreeItemBase* root);
    virtual void saveOpenness();

    /** Call this if and when the displayed content has changed. */
    void updateContent()
    {
        if (rootItem)
            rootItem->treeHasChanged();
    }

    void deleteSelectedItems()
    {
        if (rootItem != nullptr)
            rootItem->deleteAllSelectedItems();
    }

    void setEmptyTreeMessage (const String& newMessage)
    {
        if (emptyTreeMessage.getValue().toString() != newMessage)
        {
            emptyTreeMessage.setValue (newMessage);
            repaint();
        }
    }

    static void drawEmptyPanelMessage (Component& comp, Graphics& g, const String& message)
    {
        const int fontHeight = 13;
        const Rectangle<int> area (comp.getLocalBounds());
        g.setColour (Colours::black.contrasting (0.7f));
        g.setFont ((float) fontHeight);
        g.drawFittedText (message, area.reduced (4, 2), Justification::centred, area.getHeight() / fontHeight);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colors::contentBackgroundColor);
        if (! emptyTreeMessage.getValue().isUndefined() && (rootItem == nullptr || rootItem->getNumSubItems() == 0))
            drawEmptyPanelMessage (*this, g, emptyTreeMessage.getValue());
    }

    void resized() override
    {
        tree.setBounds (getAvailableBounds());
    }

    Rectangle<int> getAvailableBounds() const
    {
        return Rectangle<int> (0, 2, getWidth() - 2, getHeight() - 2);
    }

    TreeView tree;
    std::unique_ptr<TreeItemBase> rootItem;

private:
    String opennessStateKey;
    Value emptyTreeMessage;
};

class TreeItemComponent : public Component
{
public:
    TreeItemComponent (TreeItemBase& i)
        : item (i)
    {
        setInterceptsMouseClicks (false, true);
    }

    virtual ~TreeItemComponent() {}

    void paint (Graphics& g) override
    {
        g.setColour (Colours::black);
        paintIcon (g);
        item.paintContent (g, Rectangle<int> (item.textX, 0, getWidth() - item.textX, getHeight()));
    }

    void paintIcon (Graphics& g)
    {
        item.getIcon().draw (g, Rectangle<float> (4.0f, 2.0f, item.getIconSize(), getHeight() - 4.0f), item.isIconCrossedOut());
    }

    void resized() override
    {
        item.textX = (int) item.getIconSize() + 8;
    }

    TreeItemBase& item;
};

} // namespace element

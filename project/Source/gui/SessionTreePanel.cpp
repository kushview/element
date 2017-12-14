/*
    SessionTreePanel.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "gui/GuiCommon.h"
#include "gui/ContentComponent.h"
#include "gui/SessionTreePanel.h"
#include "gui/ViewHelpers.h"
#include "session/Node.h"

namespace Element {

SessionGraphsListBox::SessionGraphsListBox (Session* s)
    : session (nullptr)
{
    setModel (this);
    updateContent();
}
    
SessionGraphsListBox::~SessionGraphsListBox()
{
    setModel (nullptr);
    session = nullptr;
}

int SessionGraphsListBox::getNumRows()
{
    return (session) ? session->getNumGraphs() : 0;
}
    
void SessionGraphsListBox::paintListBoxItem (int rowNumber, Graphics& g, int width, int height,
                                             bool rowIsSelected)
{
    if (! session)
        return;
    const Node node (session->getGraph (rowNumber));
    ViewHelpers::drawBasicTextRow ("  " + node.getName(), g, width, height, rowIsSelected);
}

class SessionNodeTreeItem : public TreeItemBase
{
public:
    SessionNodeTreeItem (const Node& n) : node (n) { }
    
    void itemOpennessChanged (const bool isOpen) override
    {
        if (isOpen)
            refreshSubItems();
        else
            clearSubItems();
    }

    void addSubItems() override
    {
        const auto nodes (node.getNodesValueTree());
        for (int i = 0; i < nodes.getNumChildren(); ++i)
            addSubItem (new SessionNodeTreeItem (Node (nodes.getChild(i), false)));
    }

    bool mightContainSubItems() override            { return node.isGraph(); }
    String getRenamingName() const override         { return getDisplayName(); }
    String getDisplayName() const override          { return node.getName(); }
    void setName (const String& newName) override   { ignoreUnused (newName); }
    bool isMissing() { return false; }
    Icon getIcon() const override { 
        return node.isGraph() ? Icon (getIcons().folder, Colours::orange)
                              : Icon (getIcons().jigsaw, Colors::elemental); 
    }

    Node node;
};

class SessionRootTreeItem : public TreeItemBase
{
public:
    SessionRootTreeItem (SessionTreePanel& o) : panel (o)
    {

    }

    void itemOpennessChanged (const bool isOpen) override
    {
        refreshSubItems();
    }

    void addSubItems() override
    {
        if (auto session = panel.getSession())
        {
            for (int i = 0; i < session->getNumGraphs(); ++i)
                addSubItem (new SessionNodeTreeItem (session->getGraph (i)));
        }
    }

    virtual bool mightContainSubItems() override { return true; }
    virtual String getRenamingName() const override { return getDisplayName(); }
    virtual String getDisplayName() const override { return "Session"; }
    virtual void setName (const String& newName) override { }
    virtual bool isMissing() override { return false; }
    virtual Icon getIcon() const override { return Icon (getIcons().folder, Colours::red); }

    SessionTreePanel& panel;
};

SessionTreePanel::SessionTreePanel()
    : TreePanelBase ("session")
{
    tree.setRootItemVisible (false);
    tree.setInterceptsMouseClicks (true, true);
    setRoot (new SessionRootTreeItem (*this));
}

SessionTreePanel::~SessionTreePanel()
{
    setRoot (nullptr);
}

void SessionTreePanel::mouseDown (const MouseEvent &ev)
{

}

void SessionTreePanel::setSession (SessionPtr s)
{
    session = s;
    setRoot (nullptr);
    setRoot (new SessionRootTreeItem (*this));
}

SessionPtr SessionTreePanel::getSession() const
{
    return session;
}

}

/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#pragma once

#include "ElementApp.h"
#include "session/Node.h"

namespace Element {

class ContentComponent;
class GuiController;
class NodeObject;

/** A desktop window containing a plugin's UI. */
class PluginWindow : public DocumentWindow,
                     private Value::Listener
{
public:
    struct Settings
    {
        Colour backgroundColor;
        int titleBarHeight;
    };
    
    ~PluginWindow();

    ContentComponent* getElementContentComponent() const;
    
    Toolbar* getToolbar() const;
    void updateGraphNode (NodeObject* newNode, Component* newEditor);
    Node getNode() const { return node; }
    void restoreAlwaysOnTopState();
    void moved() override;
    void closeButtonPressed() override;
    void resized() override;

    void activeWindowStatusChanged() override;

	int getDesktopWindowStyleFlags() const override
    {
		return DocumentWindow::getDesktopWindowStyleFlags() |
               ComponentPeer::windowHasCloseButton |
			   ComponentPeer::windowHasTitleBar |
               ComponentPeer::windowHasDropShadow;
	}
protected:
    PluginWindow (GuiController&, Component* const uiComp, const Node& node);

private:
    GuiController& gui;
    friend class WindowManager;
    NodeObject* owner;
    Node node;
    Value name;

    void valueChanged (Value& value) override
    {
        if (value.refersToSameSourceAs (name))
            setName (node.getDisplayName());
    }
};

}

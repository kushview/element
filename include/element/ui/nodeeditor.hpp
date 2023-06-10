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

#include <element/juce/gui_basics.hpp>
#include <element/nodeobject.hpp>
#include <element/node.hpp>

#define EL_NODE_EDITOR_DEFAULT_ID "el.DefaultNodeEditor"

namespace element {

class NodeEditorComponent : public Component {
protected:
    NodeEditorComponent (const Node&) noexcept;

public:
    NodeEditorComponent() = delete;
    virtual ~NodeEditorComponent() override;
    inline Node getNode() const { return node; }
    bool isRunningInPluginWindow() const;

protected:
    inline NodeObject* getNodeObject() const { return node.getObject(); }
    template <class T>
    inline T* getNodeObjectOfType() const
    {
        return dynamic_cast<T*> (getNodeObject());
    }

private:
    Node node;
};

} // namespace element

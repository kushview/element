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

namespace Element {

class Controller
{
public:
    Controller() : parent (nullptr) { }
    virtual ~Controller()
    {
        parent = nullptr;
    }
    
    inline Controller* getParent() const { return parent; }
    
    inline Controller* getRoot() const
    {
        Controller* c = const_cast<Controller*> (this);
        while (c != nullptr) {
            if (nullptr == c->getParent())
                break;
            c = c->getParent();
        }
        return c;
    }
    
    template<class T> inline T* findParent() const
    {
        Controller* ctl = const_cast<Controller*> (this);
        while (nullptr != ctl)
        {
            if (auto* p = dynamic_cast<T*> (ctl->parent))
                return p;
            ctl = ctl->parent;
        }
        return nullptr;
    }
    
    template<class T> inline T* findChild() const
    {
        for (auto const* c : children)
            if (T* t = const_cast<T*> (dynamic_cast<const T*> (c)))
                return t;
        return nullptr;
    }
    
    template<class T> inline T* findSibling() const
    {
        return (parent != nullptr) ? parent->findChild<T>() : nullptr;
    }
    
    inline int getNumChildren() const { return children.size(); }
    inline Controller* getChild (const int index) const
    {
        return isPositiveAndBelow (index, children.size())
            ? children.getUnchecked (index) : nullptr;
    }
    
    inline void addChild (Controller* c)
    {
        if (auto* added = children.add (c))
            added->parent = this;
    }

    virtual void initialize()
    {
        for (auto* child : children)
            child->initialize();
    }

    virtual void saveSettings()
    {
        for (auto* child : children)
            child->saveSettings();
    }

    virtual void activate()
    {
        for (auto* child : children)
            child->activate();
    }
    
    virtual void deactivate()
    {
        for (auto* child : children)
            child->deactivate();
    }
    
    virtual void shutdown()
    {
        for (auto* child : children)
            child->shutdown();
    }

    inline const OwnedArray<Controller>& getChildren() const { return children; }
    
private:
    OwnedArray<Controller> children;
    Controller* parent;
};

}

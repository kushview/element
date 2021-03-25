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

#include "engine/NodeObject.h"

namespace Element {

class NodeProvider
{
public:
    NodeProvider() = default;
    virtual ~NodeProvider() = default;
    virtual NodeObject* create (const String&) =0;
    virtual StringArray findTypes() =0;
};

class NodeManager final
{
public:
    NodeManager();
    ~NodeManager();

    void getPluginDescriptions (OwnedArray<PluginDescription>& out, const String& identifier);

    const StringArray& getKnownIDs() const { return knownIDs; }

    NodeManager& add (NodeProvider* f);

    template<class NT>
    NodeManager& add (const String& identifier)
    {
        return add (new Single<NT> (identifier));
    }

    NodeObjectPtr instantiate (const PluginDescription&);
    NodeObjectPtr instantiate (const String& identifier);

private:
    OwnedArray<NodeProvider> providers;
    StringArray knownIDs;

    template<class NT>
    struct Single : public NodeProvider
    {
        const String ID;

        Single() = delete;
        Single (const String& inID) : ID (inID) {}
        ~Single() = default;
        
        StringArray findTypes() override {
            return StringArray (ID);
        }

        NodeObject* create (const String& nodeId) override {
            return (this->ID == nodeId) ? new NT() : nullptr;
        }
    };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeManager)
};

}

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

#include <element/nodeobject.hpp>

namespace element {

class GuiService;
class Node;

class NodeProvider {
public:
    NodeProvider() = default;
    virtual ~NodeProvider() = default;
    virtual String format() const { return "Element"; }
    virtual NodeObject* create (const String&) = 0;
    virtual StringArray findTypes() = 0;
    virtual StringArray getHiddenTypes() { return {}; }
};

class NodeFactory final {
public:
    NodeFactory();
    ~NodeFactory();

    /** Fill a list of plugin descriptions. public */
    void getPluginDescriptions (OwnedArray<PluginDescription>& out,
                                const String& identifier,
                                bool includeHidden = false);

    /** Returns a list of known Node IDs public and private. */
    const StringArray& getKnownIDs() const { return knownIDs; }

    NodeFactory& add (NodeProvider* f);

    template <class NT>
    NodeFactory& add (const String& identifier)
    {
        return add (new Single<NT> (identifier));
    }

    void addHiddenType (const String& tp)
    {
        denyIDs.addIfNotAlreadyThere (tp);
    }

    void hideAllTypes()
    {
        for (const auto& tp : knownIDs)
            denyIDs.add (tp);
        denyIDs.removeDuplicates (false);
        denyIDs.removeEmptyStrings();
    }

    void removeHiddenType (const String& tp)
    {
        auto idx = denyIDs.indexOf (tp);
        if (idx >= 0)
            denyIDs.remove (idx);
    }

    NodeObject* instantiate (const PluginDescription&);
    NodeObject* instantiate (const String& identifier);
    NodeObject* wrap (AudioProcessor*);

    const OwnedArray<NodeProvider>& getNodeProviders() const { return providers; }

private:
    OwnedArray<NodeProvider> providers;
    StringArray denyIDs;
    StringArray knownIDs;

    template <class NT>
    struct Single : public NodeProvider {
        const String ID;
        const String UI;

        Single() = delete;
        Single (const String& inID)
            : ID (inID) {}
        ~Single() = default;

        StringArray findTypes() override
        {
            return StringArray (ID);
        }

        NodeObject* create (const String& nodeId) override
        {
            return (this->ID == nodeId) ? new NT() : nullptr;
        }
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeFactory)
};

} // namespace element

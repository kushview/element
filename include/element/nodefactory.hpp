// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/processor.hpp>

namespace element {

class GuiService;
class Node;

class NodeProvider {
public:
    NodeProvider() = default;
    virtual ~NodeProvider() = default;
    virtual String format() const { return "Element"; }
    virtual Processor* create (const String&) = 0;
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

    Processor* instantiate (const PluginDescription&);
    Processor* instantiate (const String& identifier);
    Processor* wrap (AudioProcessor*);

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

        Processor* create (const String& nodeId) override
        {
            return (this->ID == nodeId) ? new NT() : nullptr;
        }
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeFactory)
};

} // namespace element

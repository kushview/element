// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/processor.hpp>

namespace element {

class GuiService;
class Node;

//==========================================================================
/** A provider of node processors. */
class NodeProvider {
public:
    NodeProvider() = default;
    virtual ~NodeProvider() = default;
    /** Return the format. */
    virtual String format() const { return "Element"; }
    /** Create the instance by ID string. */
    virtual Processor* create (const String&) = 0;
    /** return a list of types contained in this provider. */
    virtual StringArray findTypes() = 0;
    /** Return a list of types that should be hidden in the UI by default. */
    virtual StringArray getHiddenTypes() { return {}; }
};

//==========================================================================
/** Factory which creates node processors. */
class NodeFactory final {
public:
    /** Create a new node factory */
    NodeFactory();
    ~NodeFactory();

    /** Fill a list of plugin descriptions. public */
    void getPluginDescriptions (OwnedArray<PluginDescription>& out,
                                const String& identifier,
                                bool includeHidden = false);

    /** Returns a list of known Node IDs public and private. */
    const StringArray& knownIDs() const noexcept;

    //==========================================================================
    /** Add a new provider to the factory. */
    NodeFactory& add (NodeProvider* f);

    //==========================================================================
    /** Mark a type as hidden in the UI. */
    void hideType (const String& tp);
    /** Hide all types in the UI. */
    void hideAllTypes();
    /** Returns true if a type is hidden in the UI. */
    bool isTypeHidden (const String& tp) const noexcept;
    /** Remove a type from the hidden list. */
    void removeHiddenType (const String& tp);

    //==========================================================================
    /** Instantiate a node processor. */
    Processor* instantiate (const PluginDescription&);
    /** Instantiate a node processor. */
    Processor* instantiate (const String& identifier);

    /** Wrap an audio plugin instance as a node processor. */
    static Processor* wrap (AudioProcessor*);

    //==========================================================================
    /** Return the list of providers registered with this factory. */
    const OwnedArray<NodeProvider>& providers() const noexcept;

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeFactory)
};

} // namespace element

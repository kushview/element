// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

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
    virtual String format() const = 0;
    /** Create the instance by ID string. */
    virtual Processor* create (const String&) = 0;
    /** return a list of types contained in this provider. */
    virtual StringArray findTypes (const juce::FileSearchPath& path,
                                   bool recursive,
                                   bool allowAsync) = 0;
    /** Return a list of types that should be hidden in the UI by default. */
    virtual juce::StringArray getHiddenTypes() { return {}; }

    virtual juce::FileSearchPath defaultSearchPath() { return {}; }

    virtual void scan (const juce::String& fileOrID, juce::OwnedArray<juce::PluginDescription>& out) {}
};

//==========================================================================
/** Factory which creates node processors. */
class NodeFactory final {
public:
    /** Create a new node factory */
    NodeFactory();
    ~NodeFactory();

    /** Fill a list of Element type plugin descriptions. public */
    void getPluginDescriptions (juce::OwnedArray<juce::PluginDescription>& out,
                                const juce::String& identifier,
                                bool includeHidden = false);

    /** Fill a list of plugin descriptions. public */
    void getPluginDescriptions (juce::OwnedArray<juce::PluginDescription>& out,
                                const juce::String& format,
                                const juce::String& identifier,
                                bool includeHidden = false);

    /** Returns a list of known Node IDs public and private. */
    const juce::StringArray& knownIDs() const noexcept;

    //==========================================================================
    /** Add a new provider to the factory. */
    NodeFactory& add (NodeProvider* f);

    //==========================================================================
    /** Mark a type as hidden in the UI. */
    void hideType (const juce::String& tp);
    /** Hide all types in the UI. */
    void hideAllTypes();
    /** Returns true if a type is hidden in the UI. */
    bool isTypeHidden (const juce::String& tp) const noexcept;
    /** Remove a type from the hidden list. */
    void removeHiddenType (const juce::String& tp);

    //==========================================================================
    /** Instantiate a node processor. */
    Processor* instantiate (const juce::PluginDescription&);
    /** Instantiate a node processor. */
    Processor* instantiate (const juce::String& identifier);

    /** Wrap an audio plugin instance as a node processor. */
    static Processor* wrap (juce::AudioProcessor*);

    //==========================================================================
    /** Return the list of providers registered with this factory. */
    const juce::OwnedArray<NodeProvider>& providers() const noexcept;

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeFactory)
};

} // namespace element

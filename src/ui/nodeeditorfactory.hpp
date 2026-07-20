// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/processor.hpp>
#include <element/ui/nodeeditor.hpp>

namespace element {

class GuiService;
class Node;

enum struct NodeEditorPlacement : uint32_t
{
    PluginWindow = 1 << 0,
    NavigationPanel = 1 << 1
};

struct NodeEditorDescription
{
    juce::String ID;
    NodeEditorPlacement placement;
    juce::StringArray nodes;
};

class NodeEditorSource
{
public:
    NodeEditorSource() = default;
    virtual ~NodeEditorSource() = default;
    virtual NodeEditor* instantiate (const juce::String& identifier, const Node& node, NodeEditorPlacement placement) = 0;
    virtual void findEditors (juce::Array<NodeEditorDescription>&) = 0;
};

class NodeEditorFactory final
{
public:
    NodeEditorFactory();
    NodeEditorFactory (GuiService& g);
    ~NodeEditorFactory();

    const juce::Array<NodeEditorDescription>& getEditors() const;

    void add (NodeEditorSource* source);

    /** Register an explicit editor for the given node and editor types */
    template <class ET>
    void add (const juce::String& editorType, const juce::String& nodeType, NodeEditorPlacement placement = NodeEditorPlacement::PluginWindow)
    {
        add (new Single<ET> (editorType, nodeType, placement));
    }

    /** Register a default editor for the given node and editor types */
    template <class ET>
    void add (const juce::String& nodeType, NodeEditorPlacement placement = NodeEditorPlacement::PluginWindow)
    {
        add (new Single<ET> (EL_NODE_EDITOR_DEFAULT_ID, nodeType, placement));
    }

    /** Create the active or default editor for the given node and placement */
    std::unique_ptr<NodeEditor> instantiate (const Node& node, NodeEditorPlacement placement);

    static std::unique_ptr<juce::Component> createEditor (const Node& node);

    /** Returns the editor sources used by this factory */
    const juce::OwnedArray<NodeEditorSource>& getSources() const;

    /** Create an AudioProcessorEditor if the node has one */
    static std::unique_ptr<juce::AudioProcessorEditor> createAudioProcessorEditor (const Node&);

private:
    juce::OwnedArray<NodeEditorSource> sources;
    std::unique_ptr<NodeEditorSource> fallback;
    juce::Array<NodeEditorDescription> editors;

    template <class ET>
    class Single : public NodeEditorSource
    {
    public:
        Single (const juce::String& inID, const juce::String& nodeID, NodeEditorPlacement placement)
        {
            desc.ID = inID;
            desc.placement = placement;
            desc.nodes.add (nodeID);
        }

        NodeEditor* instantiate (const juce::String& identifier, const Node& node, NodeEditorPlacement placement) override
        {
            if (desc.placement == placement && desc.ID == identifier && desc.nodes.contains (node.getIdentifier().toString()))
            {
                auto* editor = new ET (node);
                return editor;
            }

            return nullptr;
        }

        void findEditors (juce::Array<NodeEditorDescription>& out) override
        {
            out.add (desc);
        }

    private:
        NodeEditorDescription desc;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Single)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeEditorFactory)
};

} // namespace element

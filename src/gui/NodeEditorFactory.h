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

#include "engine/nodeobject.hpp"
#include "gui/nodes/GenericNodeEditor.h"

namespace Element {

class GuiController;
class Node;

enum struct NodeEditorPlacement : uint32_t
{
    PluginWindow = 1 << 0,
    NavigationPanel = 1 << 1
};

struct NodeEditorDescription
{
    String ID;
    NodeEditorPlacement placement;
    StringArray nodes;
};

class NodeEditorSource
{
public:
    NodeEditorSource() = default;
    virtual ~NodeEditorSource() = default;
    virtual NodeEditorComponent* instantiate (const String& identifier, const Node& node, NodeEditorPlacement placement) = 0;
    virtual void findEditors (Array<NodeEditorDescription>&) = 0;
};

class NodeEditorFactory final
{
public:
    NodeEditorFactory();
    NodeEditorFactory (GuiController& g);
    ~NodeEditorFactory();

    const Array<NodeEditorDescription>& getEditors() const { return editors; }

    void add (NodeEditorSource* source)
    {
        jassert (! sources.contains (source));
        sources.add (source);
        source->findEditors (editors);
    }

    /** Register an explicit editor for the given node and editor types */
    template <class ET>
    void add (const String& editorType, const String& nodeType, NodeEditorPlacement placement = NodeEditorPlacement::PluginWindow)
    {
        add (new Single<ET> (editorType, nodeType, placement));
    }

    /** Register a default editor for the given node and editor types */
    template <class ET>
    void add (const String& nodeType, NodeEditorPlacement placement = NodeEditorPlacement::PluginWindow)
    {
        add (new Single<ET> (EL_NODE_EDITOR_DEFAULT_ID, nodeType, placement));
    }

    /** Create the active or default editor for the given node and placement */
    std::unique_ptr<NodeEditorComponent> instantiate (const Node& node, NodeEditorPlacement placement)
    {
        std::unique_ptr<NodeEditorComponent> ed;
        for (auto* s : sources)
            if (auto* e = s->instantiate (EL_NODE_EDITOR_DEFAULT_ID, node, placement))
            {
                ed.reset (e);
                break;
            }
        if (ed == nullptr && fallback)
            if (auto* e = fallback->instantiate (EL_NODE_EDITOR_DEFAULT_ID, node, placement))
                ed.reset (e);
        return ed;
    }

    /** Returns the editor sources used by this factory */
    const OwnedArray<NodeEditorSource>& getSources() const { return sources; }

    /** Create an AudioProcessorEditor if the node has one */
    static std::unique_ptr<AudioProcessorEditor> createAudioProcessorEditor (const Node&);

private:
    OwnedArray<NodeEditorSource> sources;
    std::unique_ptr<NodeEditorSource> fallback;
    Array<NodeEditorDescription> editors;

    template <class ET>
    class Single : public NodeEditorSource
    {
    public:
        Single (const String& inID, const String& nodeID, NodeEditorPlacement placement)
        {
            desc.ID = inID;
            desc.placement = placement;
            desc.nodes.add (nodeID);
        }

        NodeEditorComponent* instantiate (const String& identifier, const Node& node, NodeEditorPlacement placement) override
        {
            if (desc.placement == placement && desc.ID == identifier && desc.nodes.contains (node.getIdentifier().toString()))
            {
                auto* editor = new ET (node);
                return editor;
            }

            return nullptr;
        }

        void findEditors (Array<NodeEditorDescription>& out) override
        {
            out.add (desc);
        }

    private:
        NodeEditorDescription desc;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Single)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeEditorFactory)
};

} // namespace Element

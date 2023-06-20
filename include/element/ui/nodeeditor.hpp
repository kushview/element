// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

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

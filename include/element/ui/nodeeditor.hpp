// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/gui_basics.hpp>
#include <element/processor.hpp>
#include <element/node.hpp>

#define EL_NODE_EDITOR_DEFAULT_ID "el.DefaultNodeEditor"

namespace element {

//==============================================================================
class Editor : public Component {
public:
    Editor();
    virtual ~Editor();
    bool resizable() const noexcept { return _resizable; }

protected:
    void setResizable (bool canResize);

private:
    bool _resizable { false };
};

//==============================================================================
class NodeEditor : public Editor {
protected:
    NodeEditor (const Node&) noexcept;

public:
    NodeEditor() = delete;
    virtual ~NodeEditor() override;
    inline Node getNode() const { return node; }
    bool isRunningInPluginWindow() const;

protected:
    inline Processor* getNodeObject() const { return node.getObject(); }
    template <class T>
    inline T* getNodeObjectOfType() const
    {
        return dynamic_cast<T*> (getNodeObject());
    }

private:
    Node node;
};

} // namespace element

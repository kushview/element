// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/element.h>
#include <element/node.hpp>
#include <element/script.hpp>

namespace element {

class EL_API Graph : public Node {
public:
    /** Make an invalid graph. */
    Graph() : Node() {}
    Graph (const Node& node) : Node (node.data(), false)
    {
        if (isValid()) {
            jassert (getProperty (tags::type) == types::Graph.toString());
        }
    }

    Graph (const Node& node, bool init) : Node (node.data(), init)
    {
        if (isValid()) {
            jassert (getProperty (tags::type) == types::Graph.toString());
        }
    }

    inline Script findViewScript() const noexcept
    {
        auto scripts = getScriptsValueTree();
        return scripts.getChildWithProperty (tags::type, types::View.toString());
    }

    inline bool hasViewScript() const noexcept { return findViewScript().valid(); }
};

} // namespace element

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/element.h>
#include <element/node.hpp>
#include <element/script.hpp>

namespace element {

class EL_API Graph : public Node {
public:
    Graph() : Node() {}
    Graph (const Node& node) : Node (node.getValueTree(), false) {}
    Graph (const Node& node, bool init) : Node (node.getValueTree(), init) {}

    inline Script findViewScript() const noexcept
    {
        auto scripts = getScriptsValueTree();
        return scripts.getChildWithProperty (tags::type, types::View.toString());
    }

    inline bool hasViewScript() const noexcept { return findViewScript().valid(); }
};

} // namespace element

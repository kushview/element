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
        return scripts.getChildWithProperty (tags::kind, types::View.toString());
    }

    inline bool hasViewScript() const noexcept { return findViewScript().valid(); }
};

}

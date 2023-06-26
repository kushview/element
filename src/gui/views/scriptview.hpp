#pragma once

#include <element/ui/view.hpp>
#include <element/script.hpp>

namespace element {

class Context;
class Graph;
class Node;

class ScriptView : public View
{
public:
    ScriptView() = delete;
    ScriptView (Context&, const Script& s);
    virtual ~ScriptView();

    void stabilize();
    void resized() override;

    /** Set the node to make available in the loaded script. */
    void setNode (const Node& node);

    /** Set the node used by this script.
        Can be invalid.
    */
    const Node& node() const noexcept;

    /** Returns the graph used by this script. */
    const Graph& graph() const noexcept;

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace element

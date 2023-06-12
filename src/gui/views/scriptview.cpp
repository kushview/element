#include <element/context.hpp>
#include <element/graph.hpp>
#include <element/script.hpp>

#include "gui/views/scriptview.hpp"
#include "scripting/scriptloader.hpp"
#include "sol/sol.hpp"
#include "scripting.hpp"
#include "el/object.hpp"

namespace element {

class ScriptView::Impl : public juce::Value::Listener
{
public:
    Impl (ScriptView& v, Context& c)
        : view (v), context (c), scripting (c.getScriptingEngine()), env (scripting.getLuaState(), sol::create)
    {
    }

    ~Impl()
    {
        codeValue.removeListener (this);
        if (auto pv = proxyView())
            view.removeChildComponent (pv);
    }

    void init()
    {
        codeValue.addListener (this);
        sol::state_view state (env.lua_state());
    }

    View* proxyView() const noexcept
    {
        return lua::object_userdata<element::View> (proxy);
    }

    void resized()
    {
        if (auto pv = proxyView())
        {
            pv->setBounds (view.getLocalBounds());
        }
    }

    void setScript (Script s)
    {
        if (s.getValueTree() == script.getValueTree())
            return;

        if (auto old = proxyView())
            view.removeChildComponent (old);

        script = {};
        codeValue = juce::Value();
        proxy = {};
        descriptor = {};

        ScriptLoader loader (env.lua_state());
        if (! loader.load (s.code()))
        {
            scripting.logError (loader.getErrorMessage());
            return;
        }

        descriptor = loader.call().as<sol::table>();
        if (! descriptor.valid())
            return;

        juce::Identifier kind (descriptor.get_or<std::string> ("type", "View"));

        if (kind == types::View)
            proxy = descriptor["instantiate"]();
        else if (kind == types::GraphView)
        {
            proxy = descriptor["instantiate"](graph);
        }
        else
        {
            proxy = {};
            descriptor = {};
        }

        if (proxy.valid())
        {
            codeValue = s.getPropertyAsValue (tags::code, false);
            script = s;

            if (auto pv = proxyView())
            {
                view.addAndMakeVisible (pv);
            }
        }

        resized();
    }

    void stabilize() {}

    void setNode (const Node& newNode)
    {
        if (node != newNode)
        {
            node = newNode;
            notifyNodeChanged();
        }

        Graph newGraph (node.getParentGraph());
        if (graph != newGraph)
        {
            graph = newGraph;
            notifyGraphChanged();
        }
    }

private:
    friend class ScriptView;
    ScriptView& view;
    Context& context;
    ScriptingEngine& scripting;
    Script script;
    juce::Value codeValue;
    sol::table descriptor;
    sol::table proxy;
    sol::environment env;
    Node node;
    Graph graph;

    juce::Identifier kind() const noexcept
    {
        juce::Identifier k (descriptor.get_or<std::string> ("type", "Anonymous"));
        return k;
    }

    void notifyGraphChanged()
    {
        if (! descriptor.valid())
            return;
        if (sol::safe_function f = descriptor["graph_changed"])
        {
            f (proxy, graph);
        }
    }

    void notifyNodeChanged()
    {
        if (! descriptor.valid())
            return;
        if (sol::safe_function f = descriptor["node_changed"])
        {
            f (proxy, node);
        }
    }

    void valueChanged (juce::Value& val) override
    {
        juce::ignoreUnused (val);
    }
};

ScriptView::ScriptView (Context& ctx, const Script& src)
{
    setName ("ScriptView");
    impl = std::make_unique<Impl> (*this, ctx);
    setSize (100, 100);
    impl->setScript (src);
}

ScriptView::~ScriptView()
{
    impl.reset();
}

void ScriptView::stabilizeContent() { impl->stabilize(); }
void ScriptView::resized() { impl->resized(); }
void ScriptView::setNode (const Node& node) { impl->setNode (node); }
const Node& ScriptView::node() const noexcept { return impl->node; }
const Graph& ScriptView::graph() const noexcept { return impl->graph; }

} // namespace element

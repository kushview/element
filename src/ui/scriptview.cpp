// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/context.hpp>
#include <element/graph.hpp>
#include <element/script.hpp>

#include "scripting/scriptloader.hpp"
#include "sol/sol.hpp"
#include "scripting.hpp"
#include "el/object.hpp"

#include "ui/scriptview.hpp"

namespace element {

class ScriptView::Impl : public juce::Value::Listener
{
public:
    Impl (ScriptView& v, Context& c)
        : view (v),
          context (c),
          scripting (c.scripting()),
          env (scripting.getLuaState(), sol::create)
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
        state.script (R"(
            require ('el.Node')
            require ('el.Graph')
        )");
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
        if (s.data() == script.data())
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
        {
            scripting.logError ("Descriptor is not valid");
            return;
        }

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
            scripting.logError ("Script instantiated");
            codeValue = s.getPropertyAsValue (tags::code, false);
            script = s;

            if (auto pv = proxyView())
            {
                scripting.logError ("got proxy");
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

        Graph newGraph (node.isGraph() ? node : node.getParentGraph());
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
        juce::Identifier k (descriptor.get_or<std::string> ("type", types::Anonymous.toString().toRawUTF8()));
        return k;
    }

    void notifyGraphChanged()
    {
        if (! descriptor.valid())
            return;
        if (sol::safe_function f = descriptor["graph_changed"])
        {
            sol::safe_function_result res = f (proxy, Node (graph.data(), false));
            if (! res.valid())
            {
                sol::error e = res;
                scripting.logError (e.what());
            }
        }
    }

    void notifyNodeChanged()
    {
        if (! descriptor.valid())
            return;
        if (sol::safe_function f = descriptor["node_changed"])
        {
            sol::safe_function_result res = f (proxy, node);
            if (! res.valid())
            {
                sol::error e = res;
                scripting.logError (e.what());
            }
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
    setComponentID ("el.ScriptView");
    impl = std::make_unique<Impl> (*this, ctx);
    setSize (100, 100);
    impl->init();
    impl->setScript (src);
}

ScriptView::~ScriptView()
{
    impl.reset();
}

void ScriptView::stabilize() { impl->stabilize(); }
void ScriptView::resized() { impl->resized(); }
void ScriptView::setNode (const Node& node) { impl->setNode (node); }
const Node& ScriptView::node() const noexcept { return impl->node; }
const Graph& ScriptView::graph() const noexcept { return impl->graph; }

} // namespace element

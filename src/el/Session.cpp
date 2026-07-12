// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

/// The Session Model.
// Representation of a session.
// @classmod el.Session
// @pragma nostrip

#include <algorithm>

#include <element/element.h>
#include <element/node.hpp>
#include <element/session.hpp>

#include "sol_helpers.hpp"

using namespace juce;

// clang-format off
EL_PLUGIN_EXPORT 
int luaopen_el_Session (lua_State* L)
{
    using namespace element;

    sol::state_view lua (L);
    auto M = lua.create_table();
    M.new_usertype<Session> ("Session", sol::no_constructor, 
        sol::meta_function::to_string, [](Session& self) { return lua::to_string (self, "Session"); },
        sol::meta_function::length, [] (Session* self) { return self->getNumGraphs(); },
        sol::meta_function::index, [] (Session* self, int index) {
            return isPositiveAndBelow (--index, self->getNumGraphs())
                       ? std::make_shared<Node> (self->getGraph (index).data(), false)
                       : std::shared_ptr<Node>();
        },

        /// The session's name.
        // @tfield string Session.name
        // @within Attributes
        "name", sol::property (
            [](Session& self) { return self.getName().toStdString(); },
            [](Session& self, const char* name) { self.setName (name); }
        ),

        /// The session's current tempo.
        // @tfield string Session.tempo
        // @within Attributes
        "tempo", sol::property (
            [](Session& self) { return self.getProperty(tags::tempo).toString().toStdString(); },
            [](Session& self, double tempo) { 
                tempo = std::clamp (tempo, 20.0, 999.0);
                self.setProperty (tags::tempo, tempo); 
            }
        ),

        /// Convert to an XML string.
        // @function Session:toXmlString
        // @treturn string XML string of the session data.
        "toXmlString", [] (Session& self) -> std::string {
            auto tree = self.data().createCopy();
            Node::sanitizeRuntimeProperties (tree, true);
            return tree.toXmlString().toStdString();
        },

        /// Save plugin states.
        // Call to save the state of all nodes in a session.
        // @function Session:saveState
        "saveState",    &Session::saveGraphState,

        /// Restore state in plugins.
        // Call to restore the state of all nodes in a session.
        // @function Session:restoreState
        "restoreState", &Session::restoreGraphState
    );

    sol::stack::push (L, element::lua::removeAndClear (M, "Session"));
    return 1;
}

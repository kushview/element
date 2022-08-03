
#include <iostream>

#include "element/context.hpp"
#include "element/scripting.hpp"
#include <sol/sol.hpp>

namespace element {
namespace lua {
extern void fill_builtins (PackageLoaderMap& pkgs);
}

class Scripting::State {
public:
    State() = delete;
    State (Scripting& s)
        : owner (s)
    {
        state.open_libraries (sol::lib::base, sol::lib::string);
        auto& g = state.globals();
        g.set (refkey, std::ref (*this));
        init_packages (state);
    }

    ~State()
    {
        auto& g = state.globals();
        g.set (refkey, sol::lua_nil);
    }

    operator lua_State*() const noexcept
    {
        return state.lua_state();
    }

private:
    friend class Scripting;
    Scripting& owner;
    sol::state state;
    lua::PackageLoaderMap builtins;
    lua::PackageLoaderMap packages;

    /** global table key to state reference */
    static constexpr const char* refkey = "__state";

    static State& getref (sol::state_view& view)
    {
        return view.globals()[refkey];
    }

    static void init_packages (lua_State* L)
    {
        sol::state_view view (L);
        view.open_libraries (sol::lib::package);
#if 0
        // doing this destroys preloading capabilities in lua.
        view.clear_package_loaders();
        view.add_package_loader (resolve_internal_package);
#else
        auto package = view["package"];

        const char* skey = LUA_VERSION_NUM < 502 ? "loaders" : "searchers";
        sol::table orig_searchers = package[skey];
        auto new_searchers = view.create_table();
        new_searchers.add (orig_searchers[1]);           // first searcher is the preloader
        new_searchers.add (resolve_internal_package);    // insert ours
        for (int i = 2; i <= orig_searchers.size(); ++i) // add everything after (file searchers)
            new_searchers.add (package[skey][i]);        // ..
        package[skey] = new_searchers;                   // replace them
#endif
    }

    /** Custom lua searcher handler */
    static int resolve_internal_package (lua_State* L)
    {
        sol::state_view view (L);
        auto& state = getref (view);

        if (state.builtins.empty())
            lua::fill_builtins (state.builtins);

        const auto mid = sol::stack::get<std::string> (L);
        auto it = state.builtins.find (mid);
        auto end = state.builtins.end();
        std::string msgkey = "builtins";
        if (it == end) {
            it = state.packages.find (mid);
            end = state.packages.end();
            msgkey = "packages";
        }
        if (it != end) {
            if (nullptr != it->second)
                sol::stack::push (L, it->second);
            else
                lua_pushfstring (L, "\n\tno cfunction: lua_CFunction not present: %s", mid.c_str());
        } else {
            lua_pushfstring (L, "\n\tno field %s['%s']", msgkey.c_str(), mid.c_str());
        }

        return 1;
    }
};

Scripting::Scripting()
{
    state = std::make_unique<State> (*this);
    sol::state_view view (*state);
    view.collect_garbage();
}

Scripting::~Scripting()
{
}

void Scripting::add_package (const std::string& name, lua::CFunction loader)
{
    auto& pkgs = state->packages;
    if (pkgs.find (name) == pkgs.end()) {
        pkgs.insert ({ name, loader });
        std::clog << "Scripting::add_package(): inserted " << name << std::endl;
    }
}

std::vector<std::string> Scripting::available_packages() const noexcept
{
    std::vector<std::string> v;
    for (const auto& pkg : state->builtins)
        v.push_back (pkg.first);
    for (const auto& pkg : state->packages)
        v.push_back (pkg.first);
    std::sort (v.begin(), v.end());
    return v;
}

lua_State* Scripting::root_state() const { return *state; }

} // namespace element

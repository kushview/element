// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include <element/filesystem.hpp>
#include <sol/sol.hpp>

namespace element {

static constexpr const char* MANIFEST_FILENAME = "manifest.lua";

struct Manifest
{
    Manifest() = default;
    Manifest (const Manifest&& o)
    {
        this->name = std::move (o.name);
        this->provides = std::move (o.provides);
    }

    Manifest& operator= (const Manifest& o)
    {
        name = o.name;
        provides = o.provides;
        return *this;
    }

    std::string name;
    std::vector<std::string> provides;
};

template <typename Tx>
static Manifest read_module_manifest (Tx&& bundle_path)
{
    Manifest result;
    std::filesystem::path f (bundle_path);
    f /= MANIFEST_FILENAME;
    f.make_preferred();

    sol::state state;
    state.open_libraries (sol::lib::base, sol::lib::string);

    try
    {
        state.safe_script_file (f.string());

        result.name = state.get_or ("name", std::string (""));

        auto provides = state.get_or ("provides", sol::table());
        if (provides.valid())
            for (const auto& item : provides)
                if (item.first.is<int>() && item.second.is<std::string>())
                    result.provides.push_back (item.second.as<std::string>());

    } catch (const std::exception& e)
    {
        std::clog << "config error: " << e.what() << std::endl;
    }

    return result;
}

} // namespace element

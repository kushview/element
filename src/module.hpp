
#pragma once

#include <map>
#include <string>
#include <vector>

#include "element/element.hpp"
#include "scripting.hpp"
#include "context.hpp"
#include "manifest.hpp"
#include "search_path.hpp"

namespace Element {

using FeatureMap = std::map<std::string, const void*>;

class Module {
public:
    Module (const std::string& bundle_path, Context& context, Scripting& s);
    ~Module();

    std::string name() const noexcept { return manifest.name; }

    static const std::string library_extension()
    {
#if __APPLE__
        return ".dylib";
#elif _WIN32
        return ".dll";
#else
        return ".so";
#endif
    }

    constexpr bool is_open() const noexcept
    {
        return library != nullptr && mod != nullptr && handle != nullptr;
    }

    constexpr const void* extension (const std::string& ID) const noexcept
    {
        return mod && handle && mod->extension ? mod->extension (handle, ID.c_str())
                                               : nullptr;
    }

    bool open();

    bool loaded() const noexcept { return has_loaded; }
    void load (elFeatures features);
    void unload();

    FeatureMap public_extensions() const;

    void close();

    const std::string& bundle_path() const noexcept { return m_bundle_path; }

private:
    Context& backend;
    Scripting& scripting;
    Manifest manifest;
    const std::string m_bundle_path;
    void* library = nullptr;
    const elDescriptor* mod = nullptr;
    elHandle handle;
    elDescriptorFunction f_descriptor = nullptr;
    bool has_loaded = false;
    void handle_module_extension (const elFeature& f);
};

class Modules {
public:
    using ptr_type = std::unique_ptr<Module>;
    using vector_type = std::vector<ptr_type>;
    Modules (Context& c) : backend (c) {}

    void add (ptr_type mod)
    {
        mods.push_back (std::move (mod));
    }

    void add (Module* mod) { add (ptr_type (mod)); }

    auto begin() const noexcept { return mods.begin(); }
    auto end() const noexcept { return mods.end(); }

    bool contains (const std::string& bp) const noexcept
    {
        auto result = std::find_if (begin(), end(), [bp] (const ptr_type& ptr) {
            return ptr->name() == bp;
        });

        return result != mods.end();
    }

    int discover()
    {
        if (discovered.size() > 0)
            return (int) discovered.size();

        for (auto const& entry : searchpath.find_folders (false, "*.element")) {
            Manifest manifest = read_module_manifest (entry.string());
            if (! manifest.name.empty())
                discovered.insert ({ manifest.name, entry.string() });
        }

        return (int) discovered.size();
    }

    void unload_all()
    {
        for (const auto& mod : mods) {
            mod->unload();
            mod->close();
        }
    }

private:
    friend class Context;
    Context& backend;
    vector_type mods;
    SearchPath searchpath;
    std::map<std::string, std::string> discovered;
};

} // namespace Element

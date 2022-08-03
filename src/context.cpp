
#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "element/context.hpp"
#include "element/scripting.hpp"

#include "filesystem.hpp"
#include "module.hpp"
#include <sol/sol.hpp>

namespace element {

} // namespace element

//=============================================================================
namespace fs = std::filesystem;

namespace element {

Context::Context()
{
    modules.reset (new Modules (*this));
    p_scripting.reset (new Scripting());
}

Context::~Context()
{
    p_scripting.reset();
    modules->unload_all();
    modules.reset();
}

void Context::open_module (const std::string& ID)
{
    if (modules->contains (ID))
        return;

    auto it = modules->discovered.find (ID);
    if (it == modules->discovered.end()) {
        std::clog << "module not found: " << ID << std::endl;
        return;
    }

    if (auto mod = std::make_unique<Module> (it->second, *this, scripting())) {
        if (mod->open()) {
            std::clog << "module opened: " << mod->name() << std::endl;
            modules->add (std::move (mod));
        } else {
            std::clog << "could not open module: " << mod->name() << std::endl;
        }
    }
}

void Context::load_modules()
{
    std::vector<const elFeature*> features;

    for (const auto& mod : *modules) {
        for (const auto& e : mod->public_extensions()) {
            auto f = (elFeature*) std::malloc (sizeof (elFeature));
            features.push_back (f);
            f->ID = strdup (e.first.c_str());
            f->data = (void*) e.second;
        }
    }

    auto ctxfeature = (elFeature*) malloc (sizeof (elFeature));
    ctxfeature->ID = strdup ("el.Context");
    ctxfeature->data = this;
    features.push_back (ctxfeature);
    features.push_back ((elFeature*) nullptr);
    elFeatures fptr = &features.front();

    for (const auto& mod : *modules) {
        if (! mod->loaded())
            mod->load (fptr);
    }

    for (auto f : features) {
        if (nullptr == f)
            continue;
        std::free ((void*) f->ID);
        std::free ((void*) f);
    }
    features.clear();
}

void Context::add_module_path (const std::string& path)
{
    auto& sp = modules->searchpath;
    sp.add (path);
}

void Context::discover_modules()
{
    modules->discover();
}

} // namespace element

#if 1
struct elContext : public element::Context {
    elContext() = default;
    ~elContext() = default;
};
#endif

elContext* element_context_new()
{
    return new elContext();
}

void element_context_free (elContext* ctx)
{
    delete ctx;
}

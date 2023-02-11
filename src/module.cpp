
#include <element/context.hpp>
#include "scripting.hpp"

#include "dynlib.h"
#include "filesystem.hpp"
#include "module.hpp"

namespace fs = std::filesystem;

namespace element {

Module::Module (const std::string& bp, Context& b, ScriptingEngine& s)
    : backend (b),
      scripting (s),
      m_bundle_path (bp)
{
    manifest = read_module_manifest (bundle_path());
}

Module::~Module()
{
    close();
}

bool Module::open()
{
    close();
    if (! is_open())
    {
        fs::path libfile (m_bundle_path);
        libfile /= libfile.filename()
                       .replace_extension (library_extension());
        if (! fs::exists (libfile))
            return false;
        library = element_openlib (libfile.string().c_str());

        if (library != nullptr)
        {
            f_descriptor = (elDescriptorFunction)
                element_getsym (library, "element_descriptor");
        }
        else
        {
            std::cout << "library couldn't open\n";
            if (auto str = dlerror())
            {
                std::cout << "error: " << str << std::endl;
                std::free (str);
            }
        }

        if (f_descriptor)
        {
            mod = f_descriptor();
        }
        else
        {
            std::cout << "no descriptor function\n";
        }

        if (mod && mod->create)
        {
            handle = mod->create();
        }

        if (handle && mod->extension)
        {
            std::vector<std::string> mids;
            // mids.push_back (EL_EXTENSION__Main);
            // mids.push_back (EL_EXTENSION__LuaPackages);
            // mids.push_back ("el.GraphicsDevice");
            for (auto& s : mids)
            {
                if (auto data = mod->extension (handle, s.c_str()))
                {
                    elFeature feature;
                    feature.ID = s.c_str();
                    feature.data = (void*) data;
                    handle_module_extension (feature);
                }
            }
        }
    }

    return is_open();
}

void Module::handle_module_extension (const elFeature& f)
{
    bool handled = true;
#define use_extensions 0
#if use_extensions
    if (strcmp (f.ID, EL_EXTENSION__LuaPackages) == 0)
    {
        for (auto reg = (const luaL_Reg*) f.data; reg != nullptr && reg->name != nullptr && reg->func != nullptr; ++reg)
        {
            scripting.add_package (reg->name, reg->func);
        }
    }
    else if (strcmp (f.ID, EL_EXTENSION__Main) == 0)
    {
        main = (const elMain*) f.data;
    }
    else if (strcmp (f.ID, "el.GraphicsDevice") == 0)
    {
        backend.video->load_device_descriptor ((const evgDescriptor*) f.data);
    }
    else
    {
#endif
        // clang-format off
        handled = false;
        for (const auto& ex : manifest.provides) {
            if (ex == f.ID) {
                handled = true;
                break;
            }
        }
// clang-format on
#if use_extensions
    }
#endif
#undef use_extensions

    if (! handled)
        std::clog << "unhandled module feature: " << f.ID << std::endl;
}

void Module::load (elFeatures features)
{
    if (has_loaded)
        return;

    has_loaded = true;
    if (handle && mod && mod->load)
    {
        mod->load (handle, features);
    }
}

void Module::unload()
{
    if (! has_loaded)
        return;

    has_loaded = false;
    if (handle && mod && mod->unload)
    {
        mod->unload (handle);
    }
}

FeatureMap Module::public_extensions() const
{
    FeatureMap e;
    for (const auto& exp : manifest.provides)
        if (auto data = mod->extension (handle, exp.c_str()))
            e.insert ({ exp, data });
    return e;
}

void Module::close()
{
    unload();

    if (handle != nullptr)
    {
        if (mod != nullptr && mod->destroy != nullptr)
            mod->destroy (handle);
        handle = nullptr;
    }

    mod = nullptr;
    f_descriptor = nullptr;

    if (library != nullptr)
    {
        element_closelib (library);
        library = nullptr;
    }
}

} // namespace element

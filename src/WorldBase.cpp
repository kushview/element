/*
    This file is part of Element
    Copyright (C) 2016-2019  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "Module.h"
#include "WorldBase.h"

namespace Element {

/* Function type for loading an Element module */
typedef Module* (*ModuleLoadFunc)();

#if 0
static Module* world_load_module (const File& file)
{
    if (! file.existsAsFile())
        return nullptr;

    DynamicLibrary* lib = new DynamicLibrary (file.getFullPathName());

    if (ModuleLoadFunc load_module = (ModuleLoadFunc) lib->getFunction ("element_module_load"))
    {
        if (Module* mod = load_module())
        {
            return mod;
        }
    }

    if (lib) {
        delete lib;
    }

    return nullptr;
}

static Module* world_load_module (const char* name)
{
    FileSearchPath emodPath (getenv ("ELEMENT_MODULE_PATH"));

    if (! (emodPath.getNumPaths() > 0))
    {
        Logger::writeToLog ("[element] setting module paths");
        File p = String("/usr/local/lib/element/modules");
        emodPath.add (p);
    }

    Array<File> modules;
    String module = "";
    module << name << Module::extension();
    emodPath.findChildFiles (modules, File::findFiles, false, module);

    if (modules.size() > 0)
        return world_load_module (modules.getFirst());

    return nullptr;
}

#endif

struct ModuleItem {
    ModuleItem (DynamicLibrary* l, Module* m)
        : library(l), module(m) { }
    ScopedPointer<DynamicLibrary> library;
    ScopedPointer<Module>         module;
};

typedef std::map<const String, ModuleItem*> ModuleMap;

class WorldBase::Private
{
public:
    Private() { }
    ~Private() { }

    void unloadAllModules()
    {
		ModuleMap::iterator it = mods.begin();
        while (it != mods.end())
        {
            it->second->module->unload();
            it->second->module = nullptr;
            it->second->library->close();
            deleteAndZero (it->second);
            ++it;
        }

        mods.clear();
    }

    Module* loadModule (const char* name, const File& file)
    {
        const String soName (String(name) + Module::extension());
        const File binary (file.getChildFile (soName));

        ScopedPointer<DynamicLibrary> lib = new DynamicLibrary (binary.getFullPathName());
        if (ModuleLoadFunc load_module = (ModuleLoadFunc) lib->getFunction ("element_module_load"))
        {
            if (Module* mod = load_module())
            {
                mods[name] = new ModuleItem (lib.release(), mod);
                return mod;
            }
        }

        return nullptr;
    }

    Module* loadModule (const char* name)
    {
        FileSearchPath emodPath (getenv ("ELEMENT_MODULE_PATH"));

        if (! (emodPath.getNumPaths() > 0))
        {
            Logger::writeToLog ("[element] setting module paths");
            File p = String("/usr/local/lib/element/modules");
            emodPath.add (p);
        }

        Array<File> modules;
        String module = name; module << ".element";
        emodPath.findChildFiles (modules, File::findDirectories, false, module);

        if (modules.size() > 0)
            return loadModule (name, modules.getFirst());

        return nullptr;
    }

    ModuleMap mods;
    ModuleHost host;
};


WorldBase::WorldBase (void* host)
{
    priv = new Private ();
    priv->host = host;
}

WorldBase::~WorldBase()
{
    priv = nullptr;
}

int WorldBase::executeModule (const char* name)
{
    ModuleMap::iterator mit = priv->mods.find (name);
    if (mit != priv->mods.end())
    {
        // mit->second->module->run ((WorldData) this);
    }

    return -1;
}

bool WorldBase::loadModule (const char* name)
{
    ModuleMap::iterator it = priv->mods.find(name);
    if (it != priv->mods.end())
        return true;

    if (Module* mod = priv->loadModule (name))
    {
        mod->load (this);
        return true;
    }

    return false;
}

void WorldBase::unloadModules()
{
    priv->unloadAllModules();
}

}

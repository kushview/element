/*
    Module.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_MODULE_H
#define ELEMENT_MODULE_H

namespace Element {

typedef void* ModuleHandle;
typedef void* ModuleLibrary;
typedef void* ModuleHost;

/** Abstract base class for all Element modules */
class Module
{
public:
    static inline const char* extension()
    {
        #if __APPLE__
         static const char* ext = ".dylib";
        #elif _MSC_VER
         static const char* ext = ".dll";
        #else
         static const char* ext = ".so";
        #endif
        return ext;
    }

    Module() { }
    virtual ~Module() { }

    virtual void load (ModuleHost) = 0;
    virtual void unload() { }
};

}

extern "C" {

/** Entry point for element modules */
Element::Module* element_module_load();

}

#endif // ELEMENT_MODULE_H

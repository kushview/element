/*
    Module.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

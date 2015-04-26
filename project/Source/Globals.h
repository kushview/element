/*
    Globals.h - This file is part of Element
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

#ifndef ELEMENT_GLOBALS_H
#define ELEMENT_GLOBALS_H

#include "element/Juce.h"
#include "URIs.h"

namespace Element {

    class Instrument;
    class MediaManager;
    class Programs;
    class SampleCache;
    class SamplerWorld;
    class Session;
    class Writer;


    class Settings :  public ApplicationProperties
    {
    public:

        Settings();
        ~Settings();

    };


    class Globals : public WorldBase
    {
    public:

        ScopedPointer<const URIs> uris;

        Globals();
        ~Globals();

        DeviceManager& devices();
        PluginManager& plugins();
        Settings& settings();
        SymbolMap& symbols();
        MediaManager& media();
        Session& session();

        void setEngine (Shared<Engine> engine);

    private:

        class Internal;
        ScopedPointer<Internal> impl;

    };


}

#endif // ELEMENT_GLOBALS_H

/*
    ClipFactory.h - This file is part of Element
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

#ifndef ELEMENT_CLIPFACTORY_H
#define ELEMENT_CLIPFACTORY_H

#include "ElementApp.h"

namespace Element {

class ClipData;
class ClipModel;
class ClipSource;
class Engine;

/** A clip type format.
    Subclass this to implement new ClipSources
 */
class ClipType
{
public:
    ClipType() { }
    virtual ~ClipType() { }

    virtual bool canCreateFrom (const File& file) = 0;
    virtual bool canCreateFrom (const ClipModel& model) = 0;

    virtual ClipData* createClipData (Engine& e, const ClipModel& mod) = 0;
    virtual ClipSource* createSource (Engine& e, const ClipModel& mod) = 0;
    virtual ClipSource* createSource (Engine& e, const File& file) = 0;
};

class ClipFactory
{
public:
    ClipFactory (Engine& e);
    ~ClipFactory();

    ClipSource* createSource (const ClipModel& model);

    ClipType* getType (const int32 t);
    int32 numTypes() const;
    void registerType (ClipType* type);

    void setSampleRate (const double rate);

private:
    class Impl; friend class Impl;
    ScopedPointer<Impl> impl;
};

}

#endif // ELEMENT_CLIPFACTORY_H

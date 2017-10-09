/*
    ClipFactory.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#pragma once

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

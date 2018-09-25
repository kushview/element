/*
    ClipModel.h - This file is part of Element
    Copyright (C) 2016  Kushview, LLC.  All rights reserved.

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

#ifndef EL_CLIP_MODEL_H
#define EL_CLIP_MODEL_H

#include "ElementApp.h"

namespace Element {
class ClipModel :  public ObjectModel
{
public:
    explicit ClipModel (const ValueTree& data = ValueTree()) : ObjectModel (data) { }
    ClipModel (const Identifier& type) : ObjectModel (type) { }

    ClipModel (ValueTree& data, double start, double length, double offset = 0.0f)
        : ObjectModel (data)
    {
        jassert (data.isValid());
        jassert (node().hasType (Slugs::clip));
        node().setProperty ("start", start, nullptr);
        node().setProperty ("length", length, nullptr);
        node().setProperty ("offset", offset, nullptr);
        setMissingProperties();
    }
    
    ClipModel (double start, double length, double offset = 0.0f)
        : ObjectModel (Slugs::clip)
    {
        node().setProperty ("start", start, nullptr);
        node().setProperty ("length", length, nullptr);
        node().setProperty ("offset", offset, nullptr);
        setMissingProperties();
    }
    
    ClipModel (const File& file)
        : ObjectModel (Slugs::clip)
    {
        setMissingProperties();
        objectData.setProperty ("file", file.getFullPathName(), nullptr);
    }
    
    ClipModel (const ClipModel& other)
        : ObjectModel (other.node())
    { }
    
    virtual ~ClipModel() { }
    
    inline void
    setTime (const Range<double>& time, double clipOffset = 0.0f)
    {
        startValue()  = time.getStart();
        lengthValue() = time.getLength();
        offsetValue() = clipOffset;
    }
    
    inline double start()  const { return node().getProperty ("start"); }
    inline Value startValue()    { return node().getPropertyAsValue ("start", nullptr); }
    inline double length() const { return node().getProperty (Slugs::length); }
    inline Value lengthValue()   { return node().getPropertyAsValue (Slugs::length, nullptr); }
    inline double offset() const { return node().getProperty ("offset"); }
    inline Value offsetValue()   { return node().getPropertyAsValue ("offset", nullptr); }
    inline double end()    const { return start() + length(); }
    
    inline bool isValid()  const { return node().isValid() && node().hasType (Slugs::clip); }
    
    virtual inline int32 trackIndex() const {
        ValueTree track (node().getParent());
        return track.getParent().indexOf (track);
    }
    
    virtual inline int32 hashCode() const
    {
        if (node().hasProperty (Slugs::file))
        {
            const File f (node().getProperty (Slugs::file).toString());
            return f.hashCode();
        }

        return 0;
    }

    virtual inline int64 hashCode64() const
    {
        if (node().hasProperty (Slugs::file))
        {
            const File f (node().getProperty (Slugs::file).toString());
            return f.hashCode64();
        }

        return 0;
    }

    String getTypeString() const { return node().getProperty (Slugs::type, String()); }

    inline bool operator== (const ClipModel& m) const { return node() == m.node(); }
    inline bool operator!= (const ClipModel& m) const { return node() != m.node(); }
    
private:
    
    ClipModel& operator= (const ClipModel&);
    
protected:
    virtual inline void
    setMissingProperties()
    {
        if (! objectData.isValid())
            return;

        if (! node().hasProperty("start"))
            node().setProperty ("start", 0.0f, nullptr);
        if (! node().hasProperty("length"))
            node().setProperty ("length", 1.0f, nullptr);
        if (! node().hasProperty("offset"))
            node().setProperty ("offset", 1.0f, nullptr);
    }
    
};

}

#endif // EL_CLIP_MODEL_H

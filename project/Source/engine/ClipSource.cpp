/*
    ClipSource.cpp - This file is part of Element
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

#include "engine/ClipSource.h"

namespace Element {

ClipSource::ClipSource()
    : frames (0, 0),
      looping (false)
{
    parentRate = 48000.f;
    connectValues();
}

ClipSource::~ClipSource()
{
    connectValues (true);
}

void ClipSource::connectValues (bool disconnect)
{
    if (disconnect)
    {
        start.removeListener (this);
        length.removeListener (this);
        offset.removeListener (this);
        parentRate.removeListener (this);
    }
    else
    {
        start.addListener (this);
        length.addListener (this);
        offset.addListener (this);
        parentRate.addListener (this);
    }
}

void ClipSource::setModel (const ClipModel& m)
{
    if (state == m.node())
        return;
    
    state = m.node();
    start.referTo (getModel().startValue());
    length.referTo (getModel().lengthValue());
    
    if (sdata)
        sdata->clipModelChanged (m);
}

ClipModel ClipSource::getModel() const
{
    return ClipModel (state);
}

void ClipSource::valueChanged (Value &value)
{
    if (start.refersToSameSourceAs (value) || length.refersToSameSourceAs (value))
        setTime (Range<double> (start.getValue(), length.getValue()));
    else if (parentRate.refersToSameSourceAs (value))
        setTime (Range<double> (start.getValue(), length.getValue()));
}

}

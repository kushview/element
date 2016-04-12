/*
    TrackModel.h - This file is part of Element
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

#ifndef EL_TRACK_MODEL_H
#define EL_TRACK_MODEL_H

#include "element/Juce.h"

namespace Element {

class TrackModel
{
public:

    explicit TrackModel (const Identifier& valueType = "track")
        : trackData (valueType)
    { }

    TrackModel (const ValueTree& data)
        : trackData (data)
    { }

    virtual ~TrackModel() { }

    /** Test if the track is valid
        If you need additional tests for validity, override this method
    */
    inline virtual bool isValid() const { return trackData.isValid() && trackData.hasType ("track"); }

    /** Get this Track's index
        The default implementation returns the underlying ValueTree's index
        in the parent tree.  If you need a situation where track indexes can't
        correspond with the value tree child index, override this method

        The easiest thing to do is ensure the parent only contains 'tracks'
    */
    inline virtual int32 index()  const { return trackData.getParent().indexOf (trackData); }

    Value armedValue()  { return trackData.getPropertyAsValue ("armed", nullptr); }
    Value mutedValue()  { return trackData.getPropertyAsValue ("mute", nullptr); }
    Value soloedValue() { return trackData.getPropertyAsValue ("solo", nullptr); }
    Value volumeValue() { return trackData.getPropertyAsValue ("volume", nullptr); }

    void setArmed  (bool arm)   { armedValue() = arm; }
    void setMuted  (bool mute)  { mutedValue() = mute; }
    void setSoloed (bool solo)  { soloedValue() = solo; }
    void setVolume (double vol) { volumeValue() = vol; }

    Identifier treeType() const { return trackData.getType(); }
    ValueTree state() const { return trackData; }

protected:

    ValueTree trackData;
    inline void setMissingProperties()
    {
        if (! trackData.hasProperty ("armed"))
            trackData.setProperty ("armed", false, nullptr);
        if (! trackData.hasProperty ("mute"))
            trackData.setProperty ("mute", false, nullptr);
        if (! trackData.hasProperty ("solo"))
            trackData.setProperty ("solo", false, nullptr);
        if (! trackData.hasProperty ("volume"))
            trackData.setProperty ("volume", (double) 1.0f, nullptr);
    }

};
}

#endif // EL_TRACK_MODEL_H

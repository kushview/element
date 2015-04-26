/*
    Transport.h - This file is part of Element
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

#ifndef ELEMENT_TRANSPORT_H
#define ELEMENT_TRANSPORT_H

#include "element/Juce.h"

namespace Element
{

    class Transport : public Shuttle
    {
    public:

        Transport();
        ~Transport();

        inline void requestPlayState (bool p) { while (! playState.set (p)) {} }
        inline void requestRecordState (bool r) { while (! recordState.set (r)) {} }
        inline void requestTempo (const double bpm) { while (! nextTempo.set (bpm)) {} }

        void preProcess (int nframes);
        void postProcess (int nframes);

        Shared<Monitor> monitor();

    private:

        AtomicValue<bool> playState, recordState;
        AtomicValue<double> nextTempo;
        Shared<Monitor> playPos;

    };

}

#endif // ELEMENT_TRANSPORT_H

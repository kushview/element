/*
    This file is part of Element
    Copyright (C) 2018-2019  Kushview, LLC.  All rights reserved.

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

#include "Tests.h"

namespace element {

class LinearFadeTest : public UnitTestBase {
public:
    LinearFadeTest() : UnitTestBase ("LinearFade", "engine", "linearFade") {}
    virtual ~LinearFadeTest() {}

    void runTest() override
    {
        testLinearFade();
    }

private:
    void testLinearFade()
    {
        beginTest ("linear fade");
        LinearFade fader;
        fader.setSampleRate (44100.0);
        fader.setLength (0.02);
        fader.setFadesIn (false);

        expect (! fader.isActive());
        fader.startFading();
        expect (fader.isActive());

        int frame = 0;

        while (fader.isActive()) {
            float gain = fader.getNextEnvelopeValue();
            frame++;
        }

        DBG ("frames processed: " << frame);
        expect (fader.isActive() == false);
        expect (fader.getCurrentEnvelopeValue() == 0.0);
        fader.reset();
        expect (fader.getCurrentEnvelopeValue() == 1.0);
    }
};

static LinearFadeTest sLinearFadeTest;

} // namespace element

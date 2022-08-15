/*
    Author: Jatin Chowdhury (jatin@ccrma.stanford.edu)
    
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.
    
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

#pragma once

#include "engine/nodes/EQFilterProcessor.h"
#include "KnobsComponent.h"

namespace element {

class EQFilterNodeEditor : public AudioProcessorEditor
{
public:
    EQFilterNodeEditor (EQFilterProcessor& proc);
    ~EQFilterNodeEditor();

    void paint (Graphics& g) override;
    void resized() override;

private:
    EQFilterProcessor& proc; // reference to processor for this editor
    KnobsComponent knobs;

    class FreqViz : public Component
    {
    public:
        FreqViz (EQFilterProcessor& proc);
        ~FreqViz() {}

        void updateCurve();
        float getFreqForX (float xPos);
        float getXForFreq (float freq);

        void paint (Graphics& g) override;
        void resized() override;

    private:
        EQFilterProcessor& proc;
        Path curvePath; // path for frequency response curve

        const float lowFreq = 10.0f;
        const float highFreq = 22000.0f;
        const float dashLengths[2] = { 4, 1 };
    };
    FreqViz viz;
};

} // namespace element

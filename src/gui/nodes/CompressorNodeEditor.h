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

#include "engine/nodes/CompressorProcessor.h"
#include "KnobsComponent.h"

namespace Element {

class CompressorNodeEditor : public AudioProcessorEditor
{
public:
    CompressorNodeEditor (CompressorProcessor& proc);
    ~CompressorNodeEditor();

    void paint (Graphics&) override;
    void resized() override;

private:
    CompressorProcessor& proc;
    KnobsComponent knobs;

    class CompViz : public Component,
                    private CompressorProcessor::Listener,
                    private Timer
    {
    public:
        CompViz (CompressorProcessor& proc);
        ~CompViz();

        void updateInGainDB (float inDB) override;
        void timerCallback() override;

        void updateCurve();
        float getDBForX (float xPos);
        float getYForDB (float db);

        void paint (Graphics& g) override;
        void resized() override {}

    private:
        CompressorProcessor& proc;
        Path curvePath; // path for compression response curve

        // Dot coordinates
        std::atomic<float> dotX = 0.0f;
        std::atomic<float> dotY = 0.0f;

        const float lowDB = -36.0f;
        const float highDB = 6.0f;
        const float dashLengths[2] = { 4, 1 };
    };
    CompViz compViz;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorNodeEditor)
};

} // namespace Element

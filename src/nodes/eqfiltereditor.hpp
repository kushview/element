// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// Author: Jatin Chowdhury (jatin@ccrma.stanford.edu)
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "nodes/eqfiltereditor.hpp"
#include "nodes/knobs.hpp"

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

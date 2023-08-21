// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// Author: Jatin Chowdhury (jatin@ccrma.stanford.edu)
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "nodes/compressor.hpp"
#include "nodes/knobs.hpp"

namespace element {

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

} // namespace element

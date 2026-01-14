// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// Author: Jatin Chowdhury (jatin@ccrma.stanford.edu)
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/ui/style.hpp>

#include "nodes/eqfilter.hpp"
#include "nodes/eqfiltereditor.hpp"

namespace element {

EQFilterNodeEditor::FreqViz::FreqViz (EQFilterProcessor& proc)
    : proc (proc)
{
    updateCurve();
}

void EQFilterNodeEditor::FreqViz::updateCurve()
{
    curvePath.clear();

    bool started = false;
    const float scaleFactor = (float) getHeight() / 64.0f;

    // set the curve points to represent a Bode plot of the filter
    // Reference: https://ccrma.stanford.edu/~jos/spectilt/Bode_Plots.html
    for (float x = 0.0f; x < getWidth(); x += 0.5f)
    {
        auto freq = getFreqForX (x); // frequency on logarithmic axis
        auto traceMag = Decibels::gainToDecibels (proc.getMagnitudeAtFreq (freq)); // Magnitude in Decibels

        if (! started)
        {
            curvePath.startNewSubPath (x, getHeight() / 2 - traceMag * scaleFactor);
            started = true;
        }
        else
        {
            curvePath.lineTo (x, getHeight() / 2 - traceMag * scaleFactor);
        }
    }

    repaint();
}

float EQFilterNodeEditor::FreqViz::getFreqForX (float xPos)
{
    auto normX = xPos / (float) getWidth();
    return lowFreq * powf (highFreq / lowFreq, normX);
}

float EQFilterNodeEditor::FreqViz::getXForFreq (float freq)
{
    auto normX = logf (freq / lowFreq) / logf (highFreq / lowFreq);
    return normX * (float) getWidth();
}

void EQFilterNodeEditor::FreqViz::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (Style::contentBackgroundColorId));

    // draw grid
    g.setColour (Colours::grey.withAlpha (0.75f));
    auto drawHorizontalLine = [this, &g] (float height) {
        Line<float> line (0.0f, height, (float) getWidth(), height);
        g.drawDashedLine (line, dashLengths, 2);
    };

    auto yFrac = 6.0f;
    for (float y = 1.0f; y < yFrac; ++y)
        drawHorizontalLine (y * (float) getHeight() / yFrac);

    float freqsToDraw[] = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    auto drawVerticalLine = [this, &g] (float freq) {
        auto x = getXForFreq (freq);
        Line<float> line (x, 0.0f, x, (float) getHeight());
        g.drawDashedLine (line, dashLengths, 2);
    };

    for (auto freq : freqsToDraw)
        drawVerticalLine (freq);

    // draw freq response curve
    g.setColour (Colours::red);
    g.strokePath (curvePath, PathStrokeType (2.0f, PathStrokeType::JointStyle::curved));
}

void EQFilterNodeEditor::FreqViz::resized()
{
    updateCurve();
}

//==============================================================
EQFilterNodeEditor::EQFilterNodeEditor (EQFilterProcessor& proc)
    : AudioProcessorEditor (proc),
      proc (proc),
      knobs (proc, [this, &proc] { proc.updateParams(); viz.updateCurve(); }),
      viz (proc)
{
    addAndMakeVisible (knobs);
    addAndMakeVisible (viz);

    setSize (500, 400);
}

EQFilterNodeEditor::~EQFilterNodeEditor()
{
}

void EQFilterNodeEditor::paint (Graphics& g)
{
    g.fillAll (Colours::black);
}

void EQFilterNodeEditor::resized()
{
    viz.setBounds (0, 0, getWidth(), getHeight() - 100);
    knobs.setBounds (0, getHeight() - 100, getWidth(), 100);
}

} // namespace element

// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// Author: Jatin Chowdhury (jatin@ccrma.stanford.edu)
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/ui/style.hpp>

#include "nodes/compressoreditor.hpp"

namespace element {

CompressorNodeEditor::CompViz::CompViz (CompressorProcessor& proc) : proc (proc)
{
    startTimer (40);

    updateCurve();

    proc.addCompressorListener (this);
}

CompressorNodeEditor::CompViz::~CompViz()
{
    proc.removeCompressorListener (this);
}

float CompressorNodeEditor::CompViz::getDBForX (float x)
{
    auto normX = x / (float) getWidth();
    return normX * (highDB - lowDB) + lowDB;
}

float CompressorNodeEditor::CompViz::getYForDB (float db)
{
    auto normY = (db - lowDB) / (highDB - lowDB);
    return (1.0f - normY) * (float) getHeight();
}

void CompressorNodeEditor::CompViz::updateCurve()
{
    curvePath.clear();

    bool started = false;

    for (float x = 0.0f; x < (float) getWidth(); x += 0.5f)
    {
        auto inputDB = getDBForX (x);
        auto gainDB = proc.calcGainDB (inputDB);
        auto traceY = getYForDB (inputDB + gainDB);

        if (! started)
        {
            curvePath.startNewSubPath (x, traceY);
            started = true;
        }
        else
        {
            curvePath.lineTo (x, traceY);
        }
    }

    repaint();
}

void CompressorNodeEditor::CompViz::updateInGainDB (float inDB)
{
    dotX = jlimit (0.0f, (float) getWidth(), ((inDB - lowDB) / (highDB - lowDB)) * (float) getWidth());
    float outDB = proc.calcGainDB (inDB) + inDB;
    dotY = getYForDB (outDB);
}

void CompressorNodeEditor::CompViz::timerCallback()
{
    repaint();
}

void CompressorNodeEditor::CompViz::paint (Graphics& g)
{
    g.fillAll (Colors::widgetBackgroundColor.darker (0.1f));

    // draw grid
    g.setColour (Colours::grey.withAlpha (0.75f));
    float gap = (float) getWidth() / 7.0f;
    for (float x = 0; x < (float) getWidth(); x += gap)
    {
        Line<float> lineH (x, 0, x, (float) getWidth());
        g.drawDashedLine (lineH, dashLengths, 2);

        Line<float> lineV (0, x, (float) getWidth(), x);
        g.drawDashedLine (lineV, dashLengths, 2);
    }

    // draw freq response curve
    g.setColour (Colours::red);
    g.strokePath (curvePath, PathStrokeType (2.0f, PathStrokeType::JointStyle::curved));

    // draw dot
    g.setColour (Colours::orange);
    g.fillEllipse (dotX - 5, dotY - 5, 10, 10);

    // Draw outline
    g.setColour (Colours::white);
    g.drawRect (getLocalBounds().toFloat().reduced (0.5f));
}

//================================================
CompressorNodeEditor::CompressorNodeEditor (CompressorProcessor& proc) : AudioProcessorEditor (proc),
                                                                         proc (proc),
                                                                         knobs (proc, [this, &proc] { proc.updateParams(); compViz.updateCurve(); }),
                                                                         compViz (proc)
{
    setOpaque (true);
    setSize (610, 420);
    addAndMakeVisible (knobs);
    addAndMakeVisible (compViz);
}

CompressorNodeEditor::~CompressorNodeEditor()
{
}

void CompressorNodeEditor::paint (Graphics& g)
{
    g.fillAll (Colors::widgetBackgroundColor.darker (0.1f));
}

void CompressorNodeEditor::resized()
{
    compViz.setBounds ((getWidth() - 300) / 2, 5, 300, 300);
    knobs.setBounds (0, getHeight() - 100, getWidth(), 100);

    compViz.updateCurve();
}

} // namespace element

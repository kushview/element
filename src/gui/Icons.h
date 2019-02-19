/*
    Icons.h - This file is part of Element
    Copyright (C) 2015  Michael Fisher <mfisher31@gmail.com>
*/

#pragma once

#include "ElementApp.h"

namespace Element {

struct Icon
{
    Icon() : path (nullptr) { }
    Icon (const Path& p, const Colour& c)  : path (&p), colour (c) {}
    Icon (const Path* p, const Colour& c)  : path (p),  colour (c) {}

    void draw (Graphics& g, const Rectangle<float>& area, bool isCrossedOut) const
    {
        if (path != nullptr)
        {
            g.setColour (colour);

            const RectanglePlacement placement (RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize);
            g.fillPath (*path, placement.getTransformToFit (path->getBounds(), area));

            if (isCrossedOut)
            {
                g.setColour (Colours::red.withAlpha (0.8f));
                g.drawLine ((float) area.getX(), area.getY() + area.getHeight() * 0.2f,
                            (float) area.getRight(), area.getY() + area.getHeight() * 0.8f, 3.0f);
            }
        }
    }

    Icon withContrastingColourTo (const Colour& background) const
    {
        return Icon (path, background.contrasting (colour, 0.6f));
    }

    const Path* path;
    Colour colour;
};

class Icons : public DeletedAtShutdown
{
public:
    Icons();

    Path folder, document, imageDoc,
         config, exporter, juceLogo,
         graph, jigsaw, info, warning,
         bug, mainJuceLogo, 
         
         falBars, falCog, falBarsOutline, falAtomAlt,

         fasPlay, fasStop, fasCog, fasCircle,
         fasChevronDown, fasChevronRight, fasSave,
         fasFolderOpen, fasPowerOff, fasThLarge,
         fasRectangleLandscape;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Icons)
};

const Icons& getIcons();

}

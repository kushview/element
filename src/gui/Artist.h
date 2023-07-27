// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce.hpp>

namespace element {

struct Artist
{
    /** The default font size that should be used in normal situations...
        e.g. standard Labels, Textboxes, ComboBoxes, etc etc */
    static constexpr float fontSizeDefault = 12.0f;

    /** Draws text rotated by 90 or -90 degrees */
    static void drawVerticalText (Graphics& g, const String& text, const Rectangle<int> area, Justification justification = Justification::centredLeft)
    {
        auto r = area;
        Graphics::ScopedSaveState savestate (g);

        if (justification == Justification::centred)
        {
            g.setOrigin (r.getX(), r.getY());
            g.addTransform (AffineTransform().rotated (
                MathConstants<float>::pi / 2.0f, 0.0f, 0.0f));
            g.drawText (text,
                        0,
                        -r.getWidth(),
                        r.getHeight(),
                        r.getWidth(),
                        justification,
                        false);
        }
        else if (justification == Justification::left || justification == Justification::centredLeft || justification == Justification::topLeft || justification == Justification::bottomLeft)
        {
            g.setOrigin (r.getX(), r.getY());
            g.addTransform (AffineTransform().rotated (
                MathConstants<float>::pi / 2.0f, 0.0f, 0.0f));
            g.drawText (text,
                        0,
                        -r.getWidth(),
                        r.getHeight(),
                        r.getWidth(),
                        justification,
                        false);
        }
        else if (justification == Justification::right || justification == Justification::centredRight || justification == Justification::topRight || justification == Justification::bottomRight)
        {
            g.setOrigin (r.getX(), r.getY());
            g.addTransform (AffineTransform().rotated (
                -MathConstants<float>::pi / 2.0f, 0.0f, (float) r.getHeight()));
            g.drawText (text,
                        0,
                        r.getHeight(),
                        r.getHeight(),
                        r.getWidth(),
                        justification,
                        false);
        }
        else
        {
            jassertfalse; // mode not supported
        }
    }
};

} // namespace element

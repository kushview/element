/*
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

#include "JuceHeader.h"

namespace Element {

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
                float_Pi / 2.0f, 0.0f, 0.0f));
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
                float_Pi / 2.0f, 0.0f, 0.0f));
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
                -float_Pi / 2.0f, 0.0f, (float) r.getHeight()));
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

} // namespace Element

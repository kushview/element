// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/gui_basics.hpp>

namespace element {
//==============================================================================
/**
    Creates a decibel scale using iec lin 2 db scale
*/
class DecibelScale : public juce::Component {
public:
    enum DecibelLevels {
        LevelOver = 0,
        Level0dB = 1,
        Level3dB = 2,
        Level6dB = 3,
        Level10dB = 4,
        LevelCount = 5
    };

    enum ColourIds {
        markerColourId = 0x11112222
    };

    DecibelScale();
    ~DecibelScale();

    int iecScale (const float dB) const;
    int iecLevel (const int index) const;

    /** @internal */
    void paint (juce::Graphics& g);
    /** @internal */
    void resized();

protected:
    void drawLabel (juce::Graphics& g, const int y, const juce::String& label);

private:
    juce::Font font;
    float scale;
    int lastY;
    int levels[LevelCount];
};

} // namespace element

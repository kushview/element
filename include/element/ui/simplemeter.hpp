// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/gui_basics.hpp>

namespace element {

class SimpleMeter;
class SimpleMeterValue;

/** Creates VU meters with scale in decibels */
class SimpleMeterValue : public juce::Component {
public:
    SimpleMeterValue (SimpleMeter* pMeter);
    ~SimpleMeterValue();

    /** Frame value accessors. */
    void setValue (const float newValue);

    /** Value refreshment. */
    void refresh();

    /** Reset peak holder. */
    void resetPeak();

    virtual void paint (juce::Graphics& g);

    /** @internal */
    void resized();

protected:
    int getIECScale (const float dB) const;
    int getIECLevel (const int index) const;
    float getValue() const { return value; }
    int getValueHold() const { return valueHold; }
    float getValueDecay() const { return valueDecay; }
    int getPeak() const { return peak; }

private:
    friend class SimpleMeter;
    SimpleMeter* meter { nullptr };

    float value;
    int valueHold;
    float valueDecay;
    int peak;
    int peakHold;
    float peakDecay;
    int peakColor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleMeterValue)
};

/** Creates VU meters with scale in decibels */
class SimpleMeter : public juce::Component {
public:
    explicit SimpleMeter (const int numPorts = 1, bool horizontal = false);
    ~SimpleMeter();

    /** Returns the number of ports on this meter */
    int getPortCount() const;
    void setValue (const int port, const float value);
    int getIECScale (const float dB) const;
    int getIECLevel (const int index) const;
    void setPeakFalloff (const int newPeakFalloff);
    int getPeakFalloff() const;
    void refresh();
    void resetPeaks();

    bool isHorizontal() const { return horizontal; }
    bool isVertical() const { return ! horizontal; }

    enum {
        ColorOver = 0,
        Color0dB = 1,
        Color3dB = 2,
        Color6dB = 3,
        Color10dB = 4,
        LevelCount = 5,
        ColorBack = 5,
        ColorFore = 6,
        ColorCount = 7
    };

    const juce::Colour& color (const int index) const;

    enum ColoursIds {
        levelOverColourId = 0x90900001,
        level0dBColourId = 0x90900002,
        level3dBColourId = 0x90900003,
        level6dBColourId = 0x90900004,
        level10dBColourId = 0x90900005,
        backgroundColourId = 0x90900006,
        foregroundColourId = 0x90900007
    };

    /** @internal */
    void paint (juce::Graphics& g);
    /** @internal */
    void resized();

protected:
    virtual SimpleMeterValue* createSimpleMeterValue();

private:
    friend class SimpleMeterValue;

    int portCount;
    SimpleMeterValue** values;
    float scale;
    int levels[LevelCount];
    juce::Colour colors[ColorCount];
    int peakFalloff;
    bool horizontal;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleMeter)
};

} // namespace element

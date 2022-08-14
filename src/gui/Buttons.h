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

#include "ElementApp.h"
#include "gui/Icons.h"
#include "gui/LookAndFeel.h"
#include "session/node.hpp"

namespace Element {

class IconButton : public Button
{
public:
    IconButton (const String& buttonName = String());
    ~IconButton() override;
    void setIcon (Icon newIcon, float reduceSize = 4.f);

protected:
    void paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown) override;

private:
    Icon icon;
    float iconReduceSize = 4.f;
};

class SettingButton : public Button
{
public:
    SettingButton (const String& name = String()) : Button (name) {}
    ~SettingButton() override {}

    enum ColourIds
    {
        textColourId = 0x30000100,
        textDisabledColourId,
        backgroundColourId,
        backgroundOnColourId
    };

    inline void setYesNoText (const String& y, const String& n)
    {
        yes = y;
        no = n;
        repaint();
    }

    inline void setIcon (const Image& image) { icon = image; }
    inline void setPath (const Path& p, float reduceby = 2.f)
    {
        path = p;
        pathReduction = juce::roundToInt (jmax (2.f, reduceby));
    }

protected:
    virtual Colour getTextColour()
    {
        return findColour (isEnabled() ? textColourId : textDisabledColourId);
    }

    /** @internal */
    void paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown) override;

private:
    String yes = "Yes";
    String no = "No";
    Image icon;
    Path path;
    int pathReduction = 2;
};

class PanicButton : public SettingButton
{
public:
    PanicButton() { setButtonText ("!"); }
};

class PowerButton : public SettingButton
{
public:
    PowerButton()
    {
        setPath (getIcons().fasPowerOff);
    }

    ~PowerButton() {}
};

class ConfigButton : public SettingButton
{
public:
    ConfigButton() { setPath (getIcons().fasCog, 2); }
    ~ConfigButton() {}
};

class GraphButton : public SettingButton
{
public:
    GraphButton() { setPath (getIcons().fasThLarge, 2.2); }
    ~GraphButton() {}
};

class DragableIntLabel : public Component,
                         public Value::Listener
{
public:
    enum ColourIds
    {
        textColourId = 0x90000000,
        backgroundColorId,
        backgroundOnColorId
    };

    DragableIntLabel()
    {
        tempoValue.addListener (this);
        setValue (120.0);
    }

    ~DragableIntLabel()
    {
        tempoValue.removeListener (this);
    }

    void paint (Graphics& g) override
    {
        const bool isOn = false;

        g.fillAll (isOn ? Colors::toggleOrange : LookAndFeel::widgetBackgroundColor.brighter());

        String text;
        if (isEnabled() && tempoValue.toString().isNotEmpty())
        {
            const auto val = (double) tempoValue.getValue();
            if (val <= minValue && useMinMax && textWhenMinimum.isNotEmpty())
                text = textWhenMinimum;
            else
                text = String (val, places);
        }
        else
        {
            text = String (stickyValue);
        }

        if (text.isNotEmpty())
        {
            g.setFont (12.f);
            g.setColour (isEnabled() ? Colours::black : Colours::darkgrey);
            g.drawText (text, getLocalBounds(), Justification::centred);
        }

        g.setColour (LookAndFeel::widgetBackgroundColor.brighter().brighter());
        g.drawRect (0, 0, getWidth(), getHeight());
    }

    void mouseDown (const MouseEvent& ev) override
    {
        if (ev.getNumberOfClicks() == 2)
            settingLabelDoubleClicked();

        if (! isEnabled())
            return;

        lastY = ev.getDistanceFromDragStartY();
    }

    void mouseDrag (const MouseEvent& ev) override
    {
        if (! isEnabled() || ! dragable)
            return;

        const int tempo = (int) tempoValue.getValue();
        int newTempo = tempo + (lastY - ev.getDistanceFromDragStartY());

        if (tempo != newTempo)
        {
            setValue (newTempo);
        }

        lastY = ev.getDistanceFromDragStartY();
    }

    void mouseUp (const MouseEvent&) override
    {
        if (! isEnabled())
            return;
    }

    virtual void settingLabelDoubleClicked() {}

    void setDragable (const bool yn) { dragable = yn; }

    inline void setValue (double value)
    {
        if (useMinMax)
        {
            if (value < minValue)
                value = minValue;
            if (value > maxValue)
                value = maxValue;
        }

        tempoValue.setValue (value);
        repaint();
    }

    inline void setMinMax (const double newMin, const double newMax)
    {
        useMinMax = true;
        minValue = newMin;
        maxValue = newMax;
        jassert (maxValue > minValue);
        const double val = (double) tempoValue.getValue();
        if (val < minValue)
            setValue (minValue);
        if (val > maxValue)
            setValue (maxValue);
    }

    inline void setTextWhenMinimum (const String& t)
    {
        textWhenMinimum = t;
        repaint();
    }
    inline Value& getValueObject() { return tempoValue; }

    void setNumDecimalPlaces (const int numPlaces)
    {
        places = jmax (0, numPlaces);
        repaint();
    }

    /** @internal */
    inline void valueChanged (Value&) override { repaint(); }

    Value tempoValue;
    int stickyValue = 0;
    String textWhenMinimum;
    int decimalPlaces = 0;
    int lastY = 0;
    int places = 0;
    bool useMinMax = false;
    double minValue = 0.0, maxValue = 0.0;

private:
    bool dragable = true;
};

class TimeSignatureSetting : public Component
{
public:
    enum ColourIds
    {
        textColourId = 0x90000000,
        backgroundColorId,
        backgroundOnColorId
    };

    TimeSignatureSetting()
    {
        beatsPerBar.setValue (4);
        beatDivisor.setValue (BeatType::QuarterNote);
    }

    ~TimeSignatureSetting() {}

    void paint (Graphics& g) override
    {
        const bool isOn = false;

        g.fillAll (isOn ? Colors::toggleOrange : LookAndFeel::widgetBackgroundColor.brighter());

        String text = beatsPerBar.toString();
        text << " / " << String (BeatType ((BeatType::ID) (int) beatDivisor.getValue()).divisor());

        if (text.isNotEmpty())
        {
            g.setFont (12.f);
            g.setColour (isEnabled() ? Colours::black : Colours::darkgrey);
            g.drawText (text, getLocalBounds(), Justification::centred);
        }

        g.setColour (LookAndFeel::widgetBackgroundColor.brighter().brighter());
        g.drawRect (0, 0, getWidth(), getHeight());
    }

    void updateMeter (int bpb, int div, const bool notify = false)
    {
        if (bpb < 1)
            bpb = 1;
        if (bpb > 99)
            bpb = 99;

        if (bpb == (int) beatsPerBar.getValue() && (int) beatDivisor.getValue() == div)
            return;

        beatsPerBar.setValue (bpb);
        beatDivisor.setValue (div);
        if (notify)
            meterChanged();

        repaint();
    }

    virtual void meterChanged() {}
    int getBeatsPerBar() const { return (int) beatsPerBar.getValue(); }
    int getBeatType() const { return (int) BeatType::QuarterNote; } // quarter note
    int getBeatDivisor() const { return (int) beatDivisor.getValue(); }

    void mouseDown (const MouseEvent& ev) override
    {
        if (! isEnabled())
            return;

        isDraggingBeatDivisor = ev.x >= (getWidth() / 2);
        lastY = ev.getDistanceFromDragStartY();
    }

    Value& getDraggedValue() { return isDraggingBeatDivisor ? beatDivisor : beatsPerBar; }

    void mouseDrag (const MouseEvent& ev) override
    {
        if (! isEnabled())
            return;

        dragging = true;
        const bool changed = 0 != (lastY / 10) - (ev.getDistanceFromDragStartY() / 10);

        if (changed)
        {
            const int delta = lastY - ev.getDistanceFromDragStartY() > 0 ? 1 : -1;
            const int value = sanitizeValue ((int) getDraggedValue().getValue() + delta);
            getDraggedValue().setValue (value);
            repaint();
        }

        lastY = ev.getDistanceFromDragStartY();
    }

    void mouseUp (const MouseEvent&) override
    {
        if (! isEnabled())
            return;

        if (dragging)
            meterChanged();

        dragging = false;
    }

private:
    Value beatsPerBar;
    Value beatDivisor;
    bool isDraggingBeatDivisor = false;
    int stickyValue = 0;
    int decimalPlaces = 0;
    int lastY = 0;
    bool dragging = false;

    int sanitizeValue (const int value)
    {
        int iVal = value;

        if (isDraggingBeatDivisor)
        {
            if (iVal < 0)
                iVal = 0;
            if (iVal > BeatType::SixteenthNote)
                iVal = BeatType::SixteenthNote;
        }
        else
        {
            if (iVal < 1)
                iVal = 1;
            if (iVal > 99)
                iVal = 99;
        }

        return iVal;
    }
};

} // namespace Element

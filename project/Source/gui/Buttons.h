
#pragma once

#include "ElementApp.h"
#include "gui/LookAndFeel.h"

namespace Element {

class SettingButton : public Button
{
public:
    SettingButton (const String& name = String()) : Button (name) { }
    ~SettingButton() { }
    
    enum ColourIds
    {
        textColourId  =  0x90000000,
        textDisabledColourId,
        backgroundColourId,
        backgroundOnColourId
    };
    
    inline void setYesNoText (const String& y, const String& n)
    {
        yes = y; no = n;
        repaint();
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
    String no  = "No";
};

class PanicButton : public SettingButton {
public:
    PanicButton() { setButtonText ("!"); }
};

class DragableIntLabel : public Component
{
public:
    enum ColourIds
    {
        textColourId            =  0x90000000,
        backgroundColorId,
        backgroundOnColorId
    };
    
    DragableIntLabel() { tempoValue.setValue (120.0); }
    ~DragableIntLabel() { }
    
    void paint (Graphics& g) override
    {
        const bool isOn = false;
        
        g.fillAll (isOn ? Colors::toggleOrange : LookAndFeel::widgetBackgroundColor.brighter());
        
        String text;
        if (isEnabled() && tempoValue.toString().isNotEmpty())
        {
            text = String ((double) tempoValue.getValue(), 0);
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
        if (! isEnabled() || !dragable)
            return;
        
        const int tempo = (int) tempoValue.getValue();
        int newTempo = tempo + (lastY - ev.getDistanceFromDragStartY());
        
        if (tempo != newTempo)
        {
            tempoValue.setValue (newTempo);
            repaint();
        }
        
        lastY = ev.getDistanceFromDragStartY();
    }
    
    void mouseUp (const MouseEvent& ) override
    {
        if (! isEnabled())
            return;
    }
    
    virtual void settingLabelDoubleClicked() { }
    
    void setDragable (const bool yn) { dragable = yn; }
    
    Value tempoValue;
    int stickyValue = 0;
    int decimalPlaces = 0;
    int lastY = 0;
    
private:
    bool dragable = true;
};


class TimeSignatureSetting : public Component
{
public:
    enum ColourIds
    {
        textColourId            =  0x90000000,
        backgroundColorId,
        backgroundOnColorId
    };
    
    TimeSignatureSetting()
    {
        beatsPerBar.setValue (4);
        beatDivisor.setValue (BeatType::QuarterNote);
    }
    
    ~TimeSignatureSetting() { }
    
    void paint (Graphics& g) override
    {
        const bool isOn = false;
        
        g.fillAll (isOn ? Colors::toggleOrange : LookAndFeel::widgetBackgroundColor.brighter());
        
        String text = beatsPerBar.toString();
        text << " / " << String (BeatType ((BeatType::ID)(int) beatDivisor.getValue()).divisor());
        
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
        if (bpb < 1) bpb = 1;
        if (bpb > 99) bpb = 99;
        
        if (bpb == (int)beatsPerBar.getValue() && (int)beatDivisor.getValue() == div)
            return;
        
        beatsPerBar.setValue (bpb);
        beatDivisor.setValue (div);
        if (notify)
            meterChanged ();
        
        repaint();
    }
    
    virtual void meterChanged () { }
    int getBeatsPerBar()    const { return (int) beatsPerBar.getValue(); }
    int getBeatType()       const { return (int) BeatType::QuarterNote; } // quarter note
    int getBeatDivisor()    const { return (int) beatDivisor.getValue(); }
    
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
            const int value = sanitizeValue ((int)getDraggedValue().getValue() + delta);
            getDraggedValue().setValue (value);
            repaint();
        }
        
        lastY = ev.getDistanceFromDragStartY();
    }
    
    void mouseUp (const MouseEvent& ) override
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
            if (iVal < 0) iVal = 0;
            if (iVal > BeatType::SixteenthNote) iVal = BeatType::SixteenthNote;
        }
        else
        {
            if (iVal < 1) iVal = 1;
            if (iVal > 99) iVal = 99;
        }
        
        return iVal;
    }
};

}

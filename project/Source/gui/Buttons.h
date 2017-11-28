
#pragma once

#include "ElementApp.h"
#include "gui/LookAndFeel.h"

namespace Element {

class SettingButton : public Button
{
public:
    SettingButton (const String& name = String()) : Button (name) { }
    ~SettingButton() { }
    
    enum ColourIds {
        textColourId  =  0x90000000,
    };
    
    inline void setYesNoText (const String& y, const String& n)
    {
        yes = y; no = n;
        repaint();
    }
    
protected:
    virtual Colour getTextColour()
    {
        findColour (textColourId);
        return Colours::black;
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
            if (! isEnabled())
                return;
            lastY = ev.getDistanceFromDragStartY();
        }
        
        void mouseDrag (const MouseEvent& ev) override
        {
            if (! isEnabled())
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
        
        Value tempoValue;
        int stickyValue = 0;
        int decimalPlaces = 0;
        int lastY = 0;
    };
}

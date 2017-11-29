
#pragma once

#include "gui/GuiCommon.h"
#include "gui/ViewHelpers.h"
#include "engine/Transport.h"

namespace Element
{

class TempoAndMeterBar : public Component,
                         public ValueListener,
                         public Timer
{
public:
    TempoAndMeterBar()
    {
        addAndMakeVisible (extButton);

        addAndMakeVisible (tempoLabel);
        
        tempoLabel.tempoValue.addListener (this);
        extButton.getToggleStateValue().addListener (this);
        
        addAndMakeVisible (meter = new TopMeter (*this));
        
        setSize (124, 24);
    }
    
    ~TempoAndMeterBar()
    {
        extButton.getToggleStateValue().removeListener (this);
        tempoLabel.tempoValue.removeListener (this);
    }
    
    void resized() override
    {
        auto r (getLocalBounds());
        
        if (extButton.isVisible())
        {
            int w = Font(18).getStringWidth ("EXT");
            extButton.setBounds (r.removeFromLeft (w + 4));
            r.removeFromLeft (2);
        }
        
        tempoLabel.setBounds (r.removeFromLeft (46));
        r.removeFromLeft (2);
        
        meter->setBounds (r.removeFromLeft (42));
    }
    
    Value& getTempoValue()          { return tempoLabel.tempoValue; }
    Value& getExternalSyncValue()   { return extButton.getToggleStateValue(); }

    void valueChanged (Value& v) override
    {
        stabilize();
        
        if (extButton.isVisible() && v.refersToSameSourceAs (extButton.getToggleStateValue()))
        {
            if (extButton.getToggleState())
            {
                // smooth session to engine tempo
                tempoLabel.engineTempo = (float) tempoLabel.tempoValue.getValue();
                tempoLabel.repaint();
                startTimer (1000.0);
            }
            else
            {
                stopTimer();
            }
        }
        
        repaint();
    }
    
    void setUseExtButton (const bool useIt)
    {
        if (useIt == extButton.isVisible())
            return;
        
        extButton.setVisible (useIt);
        stabilize();
        if (useIt)
        {
            // smooth session to engine tempo
            tempoLabel.engineTempo = (float) tempoLabel.tempoValue.getValue();
            tempoLabel.repaint();
        }
        resized();
    }
    
    bool checkMonitor()
    {
        if (monitor == nullptr || engine == nullptr || session == nullptr)
        {
            if (auto* cc = ViewHelpers::findContentComponent (this))
            {
                session = cc->getGlobals().getSession();
                engine  = cc->getGlobals().getAudioEngine();
                if (engine)
                    monitor = engine->getTransportMonitor();
            }
        }
        
        return monitor != nullptr;
    }
    
    void timerCallback() override
    {
        if (! checkMonitor())
            return;
        
        if (extButton.getToggleState() && !tempoLabel.isEnabled() && monitor)
        {
            if (tempoLabel.engineTempo != monitor->tempo.get())
            {
                tempoLabel.engineTempo = monitor->tempo.get();
                tempoLabel.repaint();
            }
            
            meter->updateMeter (monitor->beatsPerBar.get(),
                                monitor->beatDivisor.get(), false);
        }
    }
    
    void stabilizeWithSession (const bool notify = false)
    {
        session = ViewHelpers::getSession (this);
        if (! session) return;
        meter->updateMeter (session->getProperty (Tags::beatsPerBar),
                            session->getProperty (Tags::beatDivisor), notify);
    }
    
private:
    Transport::MonitorPtr monitor;
    AudioEnginePtr engine;
    SessionPtr session;
    
    void stabilize()
    {
        if (extButton.isVisible() && extButton.getToggleState())
        {
            tempoLabel.setEnabled (false);
            meter->setEnabled (false);
        }
        else
        {
            tempoLabel.setEnabled (true);
            meter->setEnabled (true);
        }
    }
    
    class ExtButton : public Button
    {
    public:
        
        ExtButton() : Button ("ExtButton")
        {
            setButtonText ("EXT");
            setClickingTogglesState (true);
        }
        
        ~ExtButton() { }
        
    protected:
        
        void paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown) override
        {
            const bool isOn = getToggleState();
            
            g.fillAll (isOn ? Colors::toggleOrange : LookAndFeel::widgetBackgroundColor.brighter());

            if (getButtonText().isNotEmpty())
            {
                g.setFont (12.f);
                g.setColour (Colours::black);
                g.drawText (getButtonText(), getLocalBounds(), Justification::centred);
            }
            
            g.setColour (LookAndFeel::widgetBackgroundColor.brighter().brighter());
            g.drawRect (0, 0, getWidth(), getHeight());
        }
    } extButton;
    
    class TempoLabel : public Component
    {
    public:
        TempoLabel() { tempoValue.setValue (120.0); }
        ~TempoLabel() { }
        
        void paint (Graphics& g) override
        {
            const bool isOn = false;
            
            g.fillAll (isOn ? Colors::toggleOrange : LookAndFeel::widgetBackgroundColor.brighter());
            
            String text;
            if (isEnabled() && tempoValue.toString().isNotEmpty())
            {
                text = String ((double) tempoValue.getValue(), 2);
            }
            else
            {
                text = String (engineTempo, 2);
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
            if (newTempo < 20)  newTempo = 20;
            if (newTempo > 999) newTempo = 999;
            
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
        float engineTempo = 0.f;
        int lastY = 0;
    } tempoLabel;
    
    class TopMeter : public TimeSignatureSetting
    {
    public:
        TopMeter (TempoAndMeterBar& o) : owner(o) { }
        void meterChanged() override
        {
            if (owner.checkMonitor())
            {
                if (auto e = owner.engine)
                    e->setMeter (getBeatsPerBar(), getBeatDivisor());
                if (auto s = owner.session)
                {
                    s->getValueTree().setProperty (Tags::beatsPerBar, getBeatsPerBar(), 0);
                    s->getValueTree().setProperty (Tags::beatDivisor, getBeatDivisor(), 0);
                }
            }
        }
        
        TempoAndMeterBar& owner;
    };
    ScopedPointer<TopMeter> meter;
};

}

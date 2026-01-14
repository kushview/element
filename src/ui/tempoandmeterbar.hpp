// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/guicommon.hpp"
#include "ui/viewhelpers.hpp"
#include <element/transport.hpp>

namespace element {

class TempoAndMeterBar : public Component,
                         public Value::Listener,
                         public Timer
{
public:
    TempoAndMeterBar() : tapTempoButton (tempoLabel)
    {
        addAndMakeVisible (extButton);

        addAndMakeVisible (tempoLabel);

        addAndMakeVisible (tapTempoButton);

        tempoLabel.tempoValue.addListener (this);
        extButton.getToggleStateValue().addListener (this);

        meter = std::make_unique<TopMeter> (*this);
        addAndMakeVisible (meter.get());

        setSize (152, 24);
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
            GlyphArrangement glyphs;
            glyphs.addLineOfText (Font (FontOptions (18)), "EXT", 0, 0);
            int w = (int) glyphs.getBoundingBox (0, -1, true).getWidth();
            extButton.setBounds (r.removeFromLeft (w + 4));
            r.removeFromLeft (2);
        }

        tempoLabel.setBounds (r.removeFromLeft (46));
        r.removeFromLeft (2);

        GlyphArrangement glyphs;
        glyphs.addLineOfText (Font (FontOptions (18)), "TAP", 0, 0);
        int w = (int) glyphs.getBoundingBox (0, -1, true).getWidth();
        tapTempoButton.setBounds (r.removeFromLeft (w + 4));
        r.removeFromLeft (2);

        meter->setBounds (r.removeFromLeft (42));
    }

    Value& getTempoValue() { return tempoLabel.tempoValue; }
    Value& getExternalSyncValue() { return extButton.getToggleStateValue(); }

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
                session = cc->context().session();
                engine = cc->context().audio();
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

        // Update labels from monitor if EXT sync is on
        if (extButton.getToggleState())
        {
            if (! tempoLabel.isEnabled())
            {
                tempoLabel.engineTempo = monitor->tempo.get();
                tempoLabel.repaint();
            }

            if (! meter->isEnabled())
            {
                meter->updateMeter (monitor->beatsPerBar.get(),
                                    monitor->beatDivisor.get(),
                                    false);
            }
        }
    }

    void stabilizeWithSession (const bool notify = false)
    {
        session = ViewHelpers::getSession (this);
        if (! session)
            return;
        meter->updateMeter (session->getProperty (tags::beatsPerBar),
                            session->getProperty (tags::beatDivisor),
                            notify);
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
            tapTempoButton.setEnabled (false);
            // MIDI clock doesn't change the meter, only disable user
            // interaction in the plugin.
            if (auto* cc = ViewHelpers::findContentComponent (this))
                if (cc->services().getRunMode() == RunMode::Plugin)
                    meter->setEnabled (false);
        }
        else
        {
            tempoLabel.setEnabled (true);
            meter->setEnabled (true);
            tapTempoButton.setEnabled (true);
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

        ~ExtButton() override {}

    protected:
        void paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown) override
        {
            const bool isOn = getToggleState();

            g.fillAll (isOn ? Colors::toggleOrange : Colors::widgetBackgroundColor.brighter());

            if (getButtonText().isNotEmpty())
            {
                g.setFont (Font (FontOptions (12.f)));
                g.setColour (Colours::black);
                g.drawText (getButtonText(), getLocalBounds(), Justification::centred);
            }

            g.setColour (Colors::widgetBackgroundColor.brighter().brighter());
            g.drawRect (0, 0, getWidth(), getHeight());
        }
    } extButton;

    class TempoLabel : public Component
    {
    public:
        TempoLabel()
        {
            tempoValue.setValue (120.0);
            addChildComponent (tempoInput);

            tempoInput.onEscapeKey = [this]() {
                tempoInput.setVisible (false);
            };

            tempoInput.onFocusLost =
                tempoInput.onReturnKey = [this]() {
                    auto txt = tempoInput.getText().trim();
                    if (txt.isEmpty() || ! tempoInput.isShowing())
                        return;

                    const auto charptr = txt.getCharPointer();
                    auto ptr = charptr;
                    auto newTempo = CharacterFunctions::readDoubleValue (ptr);

                    if ((size_t) (ptr - charptr) == txt.getNumBytesAsUTF8())
                    {
                        if (newTempo < 20.0)
                            newTempo = 20.0;
                        if (newTempo > 999.0)
                            newTempo = 999.0;
                        tempoValue.setValue (newTempo);
                    }

                    tempoInput.setVisible (false);
                    resized();
                    repaint();
                };
        }

        ~TempoLabel()
        {
            removeChildComponent (&tempoInput);
        }

        void paint (Graphics& g) override
        {
            const bool isOn = false;

            g.fillAll (isOn ? Colors::toggleOrange : Colors::widgetBackgroundColor.brighter());

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
                g.setFont (Font (FontOptions (12.f)));
                g.setColour (isEnabled() ? Colours::black : Colours::darkgrey);
                g.drawText (text, getLocalBounds(), Justification::centred);
            }

            g.setColour (Colors::widgetBackgroundColor.brighter().brighter());
            g.drawRect (0, 0, getWidth(), getHeight());
        }

        void resized() override
        {
            if (tempoInput.isVisible())
                tempoInput.setBounds (getLocalBounds());
        }

        void mouseDown (const MouseEvent& ev) override
        {
            if (! isEnabled())
                return;

            if (ev.getNumberOfClicks() == 2)
            {
                tempoInput.setText (tempoValue.getValue().toString(), dontSendNotification);
                tempoInput.setVisible (true);
                tempoInput.selectAll();
                tempoInput.grabKeyboardFocus();
                resized();
                return;
            }

            lastY = ev.getDistanceFromDragStartY();
        }

        void mouseDrag (const MouseEvent& ev) override
        {
            if (! isEnabled())
                return;

            const int tempo = (int) tempoValue.getValue();
            int newTempo = tempo + (lastY - ev.getDistanceFromDragStartY());
            if (newTempo < 20)
                newTempo = 20;
            if (newTempo > 999)
                newTempo = 999;

            if (tempo != newTempo)
            {
                tempoValue.setValue (newTempo);
                repaint();
            }

            lastY = ev.getDistanceFromDragStartY();
        }

        void mouseUp (const MouseEvent&) override
        {
            if (! isEnabled())
                return;
        }

        Value tempoValue;
        float engineTempo = 0.f;
        int lastY = 0;

    private:
        TextEditor tempoInput;
    } tempoLabel;

    class TapTempoButton : public Button
    {
    public:
        TapTempoButton (TempoLabel& tempoLabel) : Button ("TapTempoButton"),
                                                  tempoLabel (tempoLabel)
        {
            setButtonText ("TAP");
            onClick = [this] { tempoTap(); };
        }

        ~TapTempoButton() override {}

    protected:
        void paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown) override
        {
            g.fillAll (isButtonDown ? Colors::toggleOrange : Colors::widgetBackgroundColor.brighter());

            if (getButtonText().isNotEmpty())
            {
                g.setFont (Font (FontOptions (12.f)));
                g.setColour (Colours::black);
                g.drawText (getButtonText(), getLocalBounds(), Justification::centred);
            }

            g.setColour (Colors::widgetBackgroundColor.brighter().brighter());
            g.drawRect (0, 0, getWidth(), getHeight());
        }

    private:
        void tempoTap()
        {
            const double timeNow = tapTimer.getMillisecondCounterHiRes();
            const double interval = timeNow - tapTime;

            // if it's been  a long time since the last tap, reset
            if (interval > tapTempoMaxInterval)
                tapNum = 0;
            if (tapNum == 0)
            {
                tapTime = timeNow;
                tapNum++;
                return;
            }

            // convert to BPM
            int newTempo = roundToInt ((tapNum / interval) * 60000);
            if (newTempo != tempoLabel.tempoValue)
                tempoLabel.tempoValue.setValue (newTempo);
            tapNum++;
        }

        TempoLabel& tempoLabel;
        Time tapTimer;
        double tapTime = 0.0;
        int tapNum = 0;
        const double tapTempoMaxInterval = 2000.0;
    } tapTempoButton;

    class TopMeter : public TimeSignatureSetting
    {
    public:
        TopMeter (TempoAndMeterBar& o) : owner (o) {}
        void meterChanged() override
        {
            if (owner.checkMonitor())
            {
                if (auto e = owner.engine)
                    e->setMeter (getBeatsPerBar(), getBeatDivisor());
                if (auto s = owner.session)
                {
                    s->data().setProperty (tags::beatsPerBar, getBeatsPerBar(), 0);
                    s->data().setProperty (tags::beatDivisor, getBeatDivisor(), 0);
                }
            }
        }

        TempoAndMeterBar& owner;
    };
    std::unique_ptr<TopMeter> meter;
};

} // namespace element

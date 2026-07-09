// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/guicommon.hpp"
#include "ui/viewhelpers.hpp"
#include "services/mappingservice.hpp"
#include <element/transport.hpp>

namespace element {

class TempoAndMeterBar : public Component,
                         public Value::Listener,
                         public Timer
{
public:
    TempoAndMeterBar()
    {
        addAndMakeVisible (extButton);

        addAndMakeVisible (tempoLabel);

        addAndMakeVisible (tapTempoButton);

        tempoLabel.tempoValue.addListener (this);
        extButton.getToggleStateValue().addListener (this);
        beatsPerBarValue.addListener (this);
        beatDivisorValue.addListener (this);

        meter = std::make_unique<TopMeter> (*this);
        addAndMakeVisible (meter.get());

        setSize (152, 24);
    }

    ~TempoAndMeterBar()
    {
        beatDivisorValue.removeListener (this);
        beatsPerBarValue.removeListener (this);
        extButton.getToggleStateValue().removeListener (this);
        tempoLabel.tempoValue.removeListener (this);
    }

    /** The meter numerator/divisor values, bound to the session so the widget
        tracks external changes (e.g. from the MIDI Set List node). */
    Value& getBeatsPerBarValue() { return beatsPerBarValue; }
    Value& getBeatDivisorValue() { return beatDivisorValue; }

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

    /** Flash the TAP button, e.g. when a MIDI tap-tempo mapping fires. */
    void flashTapTempo() { tapTempoButton.triggerFlash(); }

    void valueChanged (Value& v) override
    {
        stabilize();

        if (v.refersToSameSourceAs (beatsPerBarValue) || v.refersToSameSourceAs (beatDivisorValue))
        {
            // notify=false: display-only sync, so we don't re-broadcast to engine/session.
            meter->updateMeter ((int) beatsPerBarValue.getValue(),
                                (int) beatDivisorValue.getValue(),
                                false);
        }

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
                                    monitor->beatType.get(),
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
    Value beatsPerBarValue;
    Value beatDivisorValue;

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

    class TapTempoButton : public Button,
                           private Timer
    {
    public:
        TapTempoButton() : Button ("TapTempoButton")
        {
            setButtonText ("TAP");
            onClick = [this] { tempoTap(); };
        }

        ~TapTempoButton() override {}

        /** Briefly flash the button. Called when a MIDI tap-tempo mapping fires,
            so a MIDI tap gets the same visual cue as a mouse click. */
        void triggerFlash()
        {
            flashing = true;
            repaint();
            startTimer (150);
        }

    protected:
        void paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown) override
        {
            g.fillAll (flashing ? Colors::toggleYellow
                                : (isButtonDown ? Colors::toggleOrange : Colors::widgetBackgroundColor.brighter()));

            if (getButtonText().isNotEmpty())
            {
                g.setFont (Font (FontOptions (12.f)));
                g.setColour (Colours::black);
                g.drawText (getButtonText(), getLocalBounds(), Justification::centred);
            }

            g.setColour (Colors::widgetBackgroundColor.brighter().brighter());
            g.drawRect (0, 0, getWidth(), getHeight());
        }

        void mouseDown (const MouseEvent& ev) override
        {
            if (isEnabled() && ev.mods.isPopupMenu())
            {
                showContextMenu();
                return;
            }
            Button::mouseDown (ev);
        }

    private:
        bool flashing = false;

        void timerCallback() override
        {
            flashing = false;
            stopTimer();
            repaint();
        }

        // The button is only an entry point: a UI tap and MIDI taps run the same
        // shared tap-tempo logic in MappingService/MappingEngine, so no MIDI or
        // model code lives here.
        void tempoTap()
        {
            auto* maps = findMappingService();
            if (maps == nullptr)
                return;

            // In MIDI-map mode, a click arms tap-tempo capture ("map, then click
            // the thing to map") instead of tapping; the next note binds it.
            if (maps->isLearning())
                maps->learnTempo();
            else
                maps->tapTempo();
        }

        MappingService* findMappingService()
        {
            if (auto* cc = ViewHelpers::findContentComponent (this))
                return cc->services().find<MappingService>();
            return nullptr;
        }

        void showContextMenu()
        {
            auto* maps = findMappingService();
            if (maps == nullptr)
                return;

            const bool mapped = maps->hasTempoMapping();

            PopupMenu menu;
            enum
            {
                Learn = 1,
                Clear
            };

            menu.addItem (Learn, mapped ? TRANS ("Re-learn MIDI Mapping") : TRANS ("MIDI Learn Tap Tempo"));

            if (mapped)
            {
                const auto desc = maps->getTempoMappingDescription();
                menu.addItem (Clear, TRANS ("Clear MIDI Mapping") + (desc.isNotEmpty() ? " (" + desc + ")" : String()));
            }

            Component::SafePointer<TapTempoButton> self (this);
            menu.showMenuAsync (PopupMenu::Options().withTargetComponent (this),
                                [self] (int result) mutable {
                                    if (self == nullptr)
                                        return;
                                    if (auto* svc = self->findMappingService())
                                    {
                                        if (result == Learn)
                                            svc->learnTempo();
                                        else if (result == Clear)
                                            svc->clearTempoMapping();
                                    }
                                });
        }
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

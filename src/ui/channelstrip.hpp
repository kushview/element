// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/ui/decibelscale.hpp>
#include <element/ui/simplemeter.hpp>
#include <element/signals.hpp>

#include "ElementApp.h"
#include "ui/buttons.hpp"

namespace element {

class ChannelStripComponent : public Component,
                              public Button::Listener,
                              public Value::Listener,
                              public Slider::Listener
{
public:
    ChannelStripComponent();
    ~ChannelStripComponent() noexcept;

    inline SimpleMeter& getSimpleMeter() { return meter; }
    inline void setVolume (const double dB, NotificationType notify = sendNotificationAsync)
    {
        fader.setValue (dB, notify);
        if (notify == dontSendNotification)
            stabilizeContent();
    }

    inline double getVolume() const { return fader.getValue(); }

    inline void setPower (const bool powerOn, const bool notify = true)
    {
        if (powerOn == powerButton.getToggleState())
            return;
        powerButton.setToggleState (powerOn, notify ? juce::sendNotification : juce::dontSendNotification);
        if (notify)
            powerChanged();
    }

    inline void setMuted (const bool muted, const bool notify = true)
    {
        if (muted == muteButton.getToggleState())
            return;
        muteButton.setToggleState (muted, notify ? juce::sendNotification : juce::dontSendNotification);
        if (notify)
            muteChanged();
    }

    inline bool isPowerOn() const { return powerButton.getToggleState(); }
    inline bool isPowerOff() const { return ! isPowerOn(); }
    inline bool isMuted() const { return muteButton.getToggleState(); }

    inline void setMuteButtonVisible (bool visible)
    {
        muteButton.setVisible (visible);
        resized();
    }

    void setFaderSkewFactor (double factor)
    {
        fader.setSkewFactor (factor);
    }

    void setMinMaxDecibels (double minDb, double maxDb);

    /** Returns the minimum decibel value of the fader. */
    double getMinDecibels() const { return fader.getMinimum(); }

    void addButton (Component*);

    /** @internal */
    void buttonClicked (Button*) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void paint (Graphics&) override;
    /** @internal */
    void valueChanged (Value&) override;
    void sliderValueChanged (Slider* slider) override;
    void sliderDragStarted (Slider*) override {}
    void sliderDragEnded (Slider*) override {}

    Signal<void (double)> volumeChanged;
    Signal<void()> powerChanged;
    Signal<void()> muteChanged;
    Signal<void()> volumeLabelDoubleClicked;

private:
    Slider fader;
    SimpleMeter meter;
    DecibelScale scale;
    struct FaderStyle;
    std::unique_ptr<FaderStyle> _fstyle;

    class VolumeLabel : public DragableIntLabel
    {
    public:
        VolumeLabel();
        ~VolumeLabel();
        void settingLabelDoubleClicked() override;
    } volume;

    PowerButton powerButton;
    SettingButton muteButton;

    OwnedArray<Component> extraButtons;

    void stabilizeContent();
};

} // namespace element

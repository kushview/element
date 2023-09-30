// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

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
        if (powerOn == mute.getToggleState())
            return;
        mute.setToggleState (powerOn, notify ? juce::sendNotification : juce::dontSendNotification);
        if (notify)
            powerChanged();
    }

    inline void setMuted (const bool muted, const bool notify = true)
    {
        if (muted == mute2.getToggleState())
            return;
        mute2.setToggleState (muted, notify ? juce::sendNotification : juce::dontSendNotification);
        if (notify)
            muteChanged();
    }

    inline bool isPowerOn() const { return mute.getToggleState(); }
    inline bool isPowerOff() const { return ! isPowerOn(); }
    inline bool isMuted() const { return mute2.getToggleState(); }

    inline void setMuteButtonVisible (bool visible)
    {
        mute2.setVisible (visible);
        resized();
    }

    void setFaderSkewFactor (double factor)
    {
        fader.setSkewFactor (factor);
    }

    void setMinMaxDecibels (double minDb, double maxDb);

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
    Label name;
    struct FaderStyle;
    std::unique_ptr<FaderStyle> _fstyle;

    class VolumeLabel : public DragableIntLabel
    {
    public:
        VolumeLabel();
        ~VolumeLabel();
        void settingLabelDoubleClicked() override;
    } volume;

    PowerButton mute;
    SettingButton mute2;

    OwnedArray<Component> extraButtons;

    void stabilizeContent();
};

} // namespace element

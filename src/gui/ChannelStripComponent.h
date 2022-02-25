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

#include "ElementApp.h"
#include "gui/Buttons.h"
#include "Signals.h"

namespace Element {

class ChannelStripComponent : public Component,
                              public Button::Listener,
                              public Value::Listener,
                              public Slider::Listener
{
public:
    ChannelStripComponent();
    ~ChannelStripComponent() noexcept;

    inline DigitalMeter& getDigitalMeter() { return meter; }
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
        mute.setToggleState (powerOn, notify);
        if (notify)
            powerChanged();
    }

    inline void setMuted (const bool muted, const bool notify = true)
    {
        if (muted == mute2.getToggleState())
            return;
        mute2.setToggleState (muted, notify);
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
    DigitalMeter meter;
    DecibelScaleComponent scale;
    Label name;
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

} // namespace Element


#include "ElementApp.h"
#include "gui/Buttons.h"
#include "Signals.h"

namespace Element {

class ChannelStripComponent : public Component,
                              public Button::Listener,
                              public Value::Listener
{
public:
    ChannelStripComponent();
    ~ChannelStripComponent() noexcept;

    inline DigitalMeter& getDigitalMeter() { return meter; }
    inline void setVolume (const double dB) { fader.setValue (dB, sendNotificationAsync); }
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

    inline bool isPowerOn()    const { return mute.getToggleState(); }
    inline bool isPowerOff()   const { return ! isPowerOn(); }
    inline bool isMuted()      const { return mute2.getToggleState(); }

    inline void setMuteButtonVisible (bool visible)
    {
        mute2.setVisible (visible);
        resized();
    }

    /** @internal */
    void buttonClicked (Button*) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void paint (Graphics&) override;
    /** @internal */
    void valueChanged (Value&) override;

    Signal<void(double)> volumeChanged;
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

    void stabilizeContent();
};

}

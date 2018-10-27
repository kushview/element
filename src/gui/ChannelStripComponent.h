
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

    inline bool isPowerOn()    const { return mute.getToggleState(); }
    inline bool isPowerOff()   const { return ! isPowerOn(); }

    /** @internal */
    void buttonClicked (Button*) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void paint (Graphics&) override;
    /** @internal */
    void valueChanged (Value&) override;

    boost::signals2::signal<void(double)> volumeChanged;
    boost::signals2::signal<void()> powerChanged;

private:
    Slider fader;
    DigitalMeter meter;
    DecibelScaleComponent scale;
    Label name;
    DragableIntLabel volume;
    PowerButton mute;

    void stabilizeContent();
};

}

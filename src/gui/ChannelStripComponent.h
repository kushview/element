
#include "ElementApp.h"
#include "gui/Buttons.h"

namespace Element {

class ChannelStripComponent : public Component,
                              public Value::Listener
{
public:
    ChannelStripComponent();
    ~ChannelStripComponent() noexcept;

    inline DigitalMeter& getDigitalMeter() { return meter; }
    inline void setVolume (const double dB) { fader.setValue (dB, sendNotificationAsync); }

    /** @internal */
    void resized() override;
    /** @internal */
    void paint (Graphics&) override;
    /** @internal */
    void valueChanged (Value&) override;

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

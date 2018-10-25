
#include "ElementApp.h"
#include "gui/Buttons.h"

namespace Element {

class ChannelStripComponent : public Component
{
public:
    ChannelStripComponent();
    ~ChannelStripComponent() noexcept;

    /** @internal */
    void resized() override;
    /** @internal */
    void paint (Graphics&) override;
private:
    Slider fader;
    DecibelScaleComponent dbScale;
    DigitalMeter meter;
    Label name, volume;
    SettingButton mute;
};

}

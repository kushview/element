#include <element/ui/meterbridge.hpp>
#include <element/services.hpp>
#include <element/context.hpp>
#include <element/devicemanager.hpp>

#include "gui/LookAndFeel.h"

namespace element {

struct SimpleLevelMeter : public Component,
                          public Timer
{
    SimpleLevelMeter (AudioDeviceManager& m) : manager (m)
    {
        startTimerHz (20);
        inputLevelGetter = manager.getInputLevelGetter();
    }

    void timerCallback() override
    {
        if (isShowing())
        {
            auto newLevel = (float) inputLevelGetter->getCurrentLevel();

            if (std::abs (level - newLevel) > 0.005f)
            {
                level = newLevel;
                repaint();
            }
        }
        else
        {
            level = 0;
        }
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
        // (add a bit of a skew to make the level more obvious)
        drawLevelMeter (g, getWidth(), getHeight(), (float) std::exp (std::log (level) / 3.0), true);
    }

    void drawLevelMeter (Graphics& g, int width, int height, float level, bool vertical)
    {
        float corner = 3.f;
        g.setColour (Colours::black); //Colours::white.withAlpha (0.7f));
        g.fillRoundedRectangle (0.0f, 0.0f, (float) width, (float) height, corner);
        g.setColour (Colours::black.withAlpha (0.2f));
        g.drawRoundedRectangle (1.0f, 1.0f, (float) width - 2.0f, (float) height - 2.0f, corner, 1.0f);

        const int totalBlocks = 7;
        const int numBlocks = roundToInt (totalBlocks * level);
        auto sz = vertical ? ((float) height - corner * 2.f) / (float) totalBlocks
                           : ((float) width - corner * 2.f) / (float) totalBlocks;

        int i2 = totalBlocks - 1;
        for (int i = 0; i < totalBlocks; ++i)
        {
            if (vertical)
            {
                if (i >= numBlocks)
                    g.setColour (Colours::lightblue.withAlpha (0.6f));
                else
                    g.setColour (i < totalBlocks - 1 ? Colours::blue.withAlpha (0.5f)
                                                    : Colours::red);
                g.fillRoundedRectangle (corner,
                                        corner + (float) i2 * sz + sz * 0.1f,
                                        (float) width - corner * 2,
                                        (float) sz * 0.8f,
                                        
                                        (float) sz * 0.4f);
                --i2;
            }
            else
            {
                if (i >= numBlocks)
                    g.setColour (Colours::lightblue.withAlpha (0.6f));
                else
                    g.setColour (i < totalBlocks - 1 ? Colours::blue.withAlpha (0.5f)
                                                 : Colours::red);

                g.fillRoundedRectangle (corner + (float) i * sz + sz * 0.1f,
                                        corner,
                                        (float) sz * 0.8f,
                                        (float) height - corner * 2,
                                        (float) sz * 0.4f);
            }
        }
    }

    AudioDeviceManager& manager;
    AudioDeviceManager::LevelMeter::Ptr inputLevelGetter;
    float level = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleLevelMeter)
};

class MeterBridge::Impl
{
public:
    Impl (MeterBridge& mb) : bridge (mb)
    {
    }

    void resized()
    {
        auto r = bridge.getLocalBounds();
        for (auto* meter : meters)
        {
            meter->setBounds (r.removeFromLeft (30));
        }
    }

    void paint (Graphics& g)
    {
        g.fillAll (LookAndFeel::widgetBackgroundColor);
    }

    void init (Context& context)
    {
        meters.clear();
        auto engine = context.getAudioEngine();

        bridge.addAndMakeVisible (meters.add (
            new SimpleLevelMeter (context.getDeviceManager())));
    }

private:
    friend class MeterBridge;
    MeterBridge& bridge;
    OwnedArray<SimpleLevelMeter> meters;
};

MeterBridge::MeterBridge (Context& ctx)
{
    setOpaque (true);
    impl = std::make_unique<Impl> (*this);
    impl->init (ctx);
    setSize (4 * 30, 80);
}

MeterBridge::~MeterBridge() {}
void MeterBridge::resized() { impl->resized(); }
void MeterBridge::paint (juce::Graphics& g) { impl->paint (g); }

//==============================================================================
MeterBridgeView::MeterBridgeView()
    : View()
{
    setName ("MeterBridgeView");
    setSize (600, 80);
}

MeterBridgeView::~MeterBridgeView() {}

void MeterBridgeView::initializeView (ServiceManager& sm)
{
    if (bridge == nullptr)
    {
        bridge = std::make_unique<MeterBridge> (sm.getWorld());
        addAndMakeVisible (bridge.get());
    }
    resized();
}

void MeterBridgeView::resized()
{
    if (bridge)
        bridge->setBounds (getLocalBounds());
}

} // namespace element

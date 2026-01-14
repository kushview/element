// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/audio_processors.hpp>
#include <element/signals.hpp>

namespace element {

class Content;
class PluginProcessor;

/** The audio processor editor used for Element plugins */
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::AsyncUpdater
{
public:
    PluginEditor (PluginProcessor&);
    ~PluginEditor();

    //==========================================================================
    Content* getContentComponent();

    //==========================================================================
    PluginProcessor& getProcessor() { return processor; }

    //==========================================================================
    void setWantsPluginKeyboardFocus (bool focus);
    bool getWantsPluginKeyboardFocus() const;

    //==========================================================================
    int getLatencySamples() const;
    void setReportZeroLatency (bool force);
    bool isReportingZeroLatency() const;

    //==========================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;
    bool keyStateChanged (bool) override { return true; }

private:
    PluginProcessor& processor;
    SafePointer<juce::Component> content;
    SignalConnection perfParamChangedConnection;

    const int paramTableSize = 100;
    bool paramTableVisible = false;
    class ParamTable;
    std::unique_ptr<ParamTable> paramTable;
    class ParamTableToggle;
    std::unique_ptr<ParamTableToggle> paramToggle;

    void updatePerformanceParamEnablements();
    bool asyncInitDone = false;
    void handleAsyncUpdate() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

} // namespace element

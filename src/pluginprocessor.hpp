// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/context.hpp>
#include <element/services.hpp>
#include <element/audioengine.hpp>
#include <element/parameter.hpp>

#include "ElementApp.h"

namespace element {

//=============================================================================
class PerformanceParameter : public juce::HostedAudioProcessorParameter,
                             public element::Parameter::Listener
{
public:
    std::function<void()> onCleared;

    explicit PerformanceParameter (int paramIdx);
    ~PerformanceParameter();

    bool haveNode() const { return node != nullptr; }
    Node getNode() const { return model; }

    juce::String getBoundParameterName() const;
    int getBoundParameter() const;

    void clearNode();
    void bindToNode (const Node& newNode, int newParam);

    /** Re-validate the bound parameter after the node's parameter set changed.

        Nodes with dynamic parameters (e.g. script nodes) rebuild their parameter
        array when ports change, orphaning the previously bound parameter. Re-resolve
        by the stored index, or clear the binding if that index no longer exists.
    */
    void nodePortsChanged();

    void setAndNotify (float value);
    void updateValue();

    //=========================================================================
    float getValue() const override;
    void setValue (float newValue) override;
    float getDefaultValue() const override;
    juce::String getParameterID() const override;
    juce::String getName (int maximumStringLength) const override;
    juce::String getLabel() const override;
    float getValueForText (const juce::String& text) const override;
    int getNumSteps() const override;
    bool isDiscrete() const override;
    bool isBoolean() const override;
    bool isMetaParameter() const override;
    juce::AudioProcessorParameter::Category getCategory() const override;
    juce::String getText (float value, int length) const override;
    bool isOrientationInverted() const override;

    //=========================================================================
    void controlValueChanged (int parameterIndex, float newValue) override;
    void controlTouched (int, bool touched) override;

private:
    juce::SpinLock lock;
    const int index;
    juce::Atomic<float> value { 0.f };
    Node model;
    ProcessorPtr node;
    juce::AudioProcessor* processor = nullptr;
    element::ParameterPtr parameter = nullptr;
    int parameterIdx = -1;
    bool special = false;
    bool recursionBlock = false;
    SignalConnection removedConnection;
    SignalConnection portsChangedConnection;
};

class PluginProcessor : public juce::AudioProcessor,
                        private juce::AsyncUpdater
{
public:
    enum Variant
    {
        Instrument,
        Effect,
        MidiEffect
    };

    explicit PluginProcessor (Variant instanceType = Instrument, int numBuses = 0);
    ~PluginProcessor() override;

    const String getName() const override;
    Variant getVariant() const { return variant; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;
    void releaseResources() override;
    void reset() override;

    bool isNodeBoundToAnyPerformanceParameter (const Node& boundNode, int boundParam) const;
    PopupMenu getPerformanceParameterMenu (int perfParam);
    void handlePerformanceParameterResult (int result, int perfParam);

    //==========================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==========================================================================
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;

    //==========================================================================
    double getTailLengthSeconds() const override;

    //==========================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==========================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    void numChannelsChanged() override;
    void numBusesChanged() override;
    void processorLayoutsChanged() override;

    Services* getServices() const { return context != nullptr ? &context->services() : nullptr; }

    void setEditorBounds (const Rectangle<int>& bounds) { editorBounds = bounds; }
    const Rectangle<int>& getEditorBounds() const { return editorBounds; }

    void updateLatencySamples();

    bool getEditorWantsKeyboard() const { return editorWantsKeyboard; }
    void setEditorWantsKeyboard (bool wantsIt) { editorWantsKeyboard = wantsIt; }

    void setForceZeroLatency (bool);
    bool isForcingZeroLatency() const { return forceZeroLatency; }

    Signal<void()> onPerfParamsChanged;

protected:
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

private:
    Array<PerformanceParameter*> perfparams;
    struct PerfParamMenuItem
    {
        Node node;
        int parameter = -1;
        bool unlink = false;
    };

    const Variant variant;

    OwnedArray<PerfParamMenuItem> menuMap;
    std::unique_ptr<Context> context;
    AudioEnginePtr engine;

    bool initialized = false;
    bool prepared = false;
    int preparedCount = 0;

    double sampleRate = 44100.0;
    int bufferSize = 512;
    int numIns = 0;
    int numOuts = 2;

    Rectangle<int> editorBounds;
    bool editorWantsKeyboard = false;

    Atomic<bool> shouldProcess = false;
    bool controllerActive = false;
    bool loadSessionOnPrepare = false;

    bool forceZeroLatency = false;

    class AsyncPrepare : public AsyncUpdater
    {
        AudioProcessor& processor;
        int bufferSize = 0;
        double sampleRate = 0.0;

    public:
        AsyncPrepare (AudioProcessor& p)
            : processor (p) {}
        ~AsyncPrepare() { cancelPendingUpdate(); }

        void prepare (double s, int b)
        {
            cancelPendingUpdate();
            bufferSize = b;
            sampleRate = s;
            triggerAsyncUpdate();
        }

        void handleAsyncUpdate() override
        {
            processor.prepareToPlay (sampleRate, bufferSize);
        }
    };

    std::unique_ptr<AsyncPrepare> asyncPrepare;

    struct Latency;
    std::unique_ptr<Latency> _latency;

    void initialize();
    friend class AsyncUpdater;
    void handleAsyncUpdate() override;
    void reloadEngine();

    int calculateLatencySamples() const;
    static BusesProperties createDefaultBuses (Variant variant, int numAux);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};

} // namespace element

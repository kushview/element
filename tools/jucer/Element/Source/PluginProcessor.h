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

#pragma once

#include "ElementApp.h"
#include "controllers/AppController.h"
#include "engine/AudioEngine.h"
#include "engine/Parameter.h"
#include "Globals.h"

using namespace Element;
using Element::Parameter;

//=============================================================================
class PerformanceParameter : public AudioProcessorParameter,
                             public Element::Parameter::Listener
{
public:
    std::function<void()> onCleared;

    explicit PerformanceParameter (int paramIdx)
        : index (paramIdx)
    {
        clearNode();
    }
    
    ~PerformanceParameter()
    {
        clearNode();
    }
    
    bool haveNode() const { return node != nullptr; }
    
    String getBoundParameterName() const
    {
        SpinLock::ScopedLockType sl (lock);
        return parameter != nullptr ? parameter->getName (100) : String();
    }

    void clearNode()
    {
        GraphNodePtr oldNode;
        Element::Parameter::Ptr oldParam;
        
        if (parameter)
            parameter->removeListener (this);
        removedConnection.disconnect();

        {
            SpinLock::ScopedLockType sl (lock);
            special         = false;
            processor       = nullptr;
            oldNode         = node;
            node            = nullptr;
            oldParam        = parameter;
            parameter       = nullptr;
            parameterIdx    = GraphNode::NoParameter;
        }
        
        oldNode.reset();
        oldParam.reset();
        model = Node();

        if (onCleared)
            onCleared();
    }
    
    void bindToNode (const Node& newNode, int newParam)
    {
        if (newNode == model)
            return;
        
        model = newNode;
        GraphNodePtr newNodeObj = model.getGraphNode();
        
        {
            SpinLock::ScopedLockType sl (lock);
            parameterIdx    = newParam;
            node            = newNodeObj;
            processor       = (node != nullptr) ? node->getAudioProcessor() : nullptr;
            parameter       = nullptr;
            if (isPositiveAndBelow (parameterIdx, node->getParameters().size()))
                parameter = node->getParameters()[parameterIdx];
        }
        
        if (node)
            removedConnection = node->willBeRemoved.connect (
                std::bind (&PerformanceParameter::clearNode, this));
        
        if (parameter != nullptr)
            parameter->addListener (this);
    }
    
    void updateValue()
    {
        if (parameter)
        {
            setValueNotifyingHost (parameter->getValue());
        }
        else
        {
            switch (parameterIdx)
            {
                case GraphNode::EnabledParameter:
                    setValueNotifyingHost (node->isEnabled() ? 1.f : 0.f);
                    break;
                case GraphNode::BypassParameter:
                    setValueNotifyingHost (node->isSuspended() ? 1.f : 0.f);
                    break;
                case GraphNode::MuteParameter:
                    setValueNotifyingHost (node->isMuted() ? 1.f : 0.f);
                    break;
            }
        }
    }
    
    float getValue() const override
    {
        SpinLock::ScopedLockType sl (lock);
        return (parameter != nullptr) ? parameter->getValue() : value.get();
    }
    
    void setValue (float newValue) override
    {
        value.set (newValue);
        SpinLock::ScopedLockType sl (lock);
        
        if (parameter != nullptr)
        {
            parameter->setValue (value.get());
        }
        else
        {
            
        }
    }
    
    float getDefaultValue() const override
    {
        SpinLock::ScopedLockType sl (lock);
        if (parameter != nullptr)
            return parameter->getDefaultValue();
        
        switch (parameterIdx)
        {
            case GraphNode::MuteParameter:      return 0.f; break;
            case GraphNode::EnabledParameter:   return 1.f; break;
            case GraphNode::BypassParameter:    return 0.f; break;
        }
        
        return 0.f;
    }
    
    String getName (int maximumStringLength) const override
    {
        String name ("Parameter "); name << int (index + 1);
        return name.substring (0, maximumStringLength);
    }
    
    String getLabel() const override
    {
        return parameter != nullptr ? parameter->getLabel() : String();
    }
    
    /** Should parse a string and return the appropriate value for it. */
    float getValueForText (const String& text) const override
    {
        return parameter != nullptr ? parameter->getValueForText (text)
            : jlimit (0.f, 1.f, text.getFloatValue());
    }
    
    int getNumSteps() const override
    {
        if (parameter != nullptr)
            return parameter->getNumSteps();
        
        switch (parameterIdx)
        {
            case GraphNode::MuteParameter:
            case GraphNode::EnabledParameter:
            case GraphNode::BypassParameter:
                return 1;
                break;
        }
        
        return AudioProcessorParameter::getNumSteps();
    }
    
    bool isDiscrete() const override
    {
        return (parameter != nullptr) ? parameter->isDiscrete()
            : AudioProcessorParameter::isDiscrete();
    }
    
    bool isBoolean() const override
    {
        if (parameter != nullptr)
            return parameter->isBoolean();
        
        switch (parameterIdx)
        {
            case GraphNode::MuteParameter:
            case GraphNode::EnabledParameter:
            case GraphNode::BypassParameter:
                return true;
                break;
        }
        
        return AudioProcessorParameter::isBoolean();
    }
    
    bool isMetaParameter() const override
    {
        return (parameter != nullptr) ? parameter->isMetaParameter()
            : AudioProcessorParameter::isMetaParameter();
    }
    
    AudioProcessorParameter::Category getCategory() const override
    {
        return (parameter != nullptr)
            ? static_cast<AudioProcessorParameter::Category> (parameter->getCategory())
            : AudioProcessorParameter::getCategory();
    }
    
    String getText (float value, int length) const override
    {
        return (parameter != nullptr)
            ? parameter->getText (value, length)
            : AudioProcessorParameter::getText (value, length);
    }
    
    bool isOrientationInverted() const override
    {
        return (parameter != nullptr) ? parameter->isOrientationInverted()
            : AudioProcessorParameter::isOrientationInverted();
    }
    
    //=========================================================================
    
    void controlValueChanged (int parameterIndex, float newValue) override
    {
        if (recursionBlock)
            return;
        ignoreUnused (parameterIndex, newValue);
        recursionBlock = true;
        updateValue();
        recursionBlock = false;
    }
    
    void controlTouched (int, bool touched) override
    {
        if (touched)
            beginChangeGesture();
        else
            endChangeGesture();
    }
    
    //=========================================================================
#if 0
    
    /** This can be overridden to tell the host that this parameter operates in the
     reverse direction.
     (Not all plugin formats or hosts will actually use this information).
     */
    virtual bool isOrientationInverted() const;
    
    /** Returns true if the host can automate this parameter.
     By default, this returns true.
     */
    virtual bool isAutomatable() const;
    
    
    //==============================================================================
    /** Returns the current value of the parameter as a String.
     
     This function can be called when you are hosting plug-ins to get a
     more specialsed textual represenation of the current value from the
     plug-in, for example "On" rather than "1.0".
     
     If you are implementing a plug-in then you should ignore this function
     and instead override getText.
     */
    virtual String getCurrentValueAsText() const;
    
    /** Returns the set of strings which represent the possible states a parameter
     can be in.
     
     If you are hosting a plug-in you can use the result of this function to
     populate a ComboBox listing the allowed values.
     
     If you are implementing a plug-in then you do not need to override this.
     */
    virtual StringArray getAllValueStrings() const;
#endif

public:
    Node getNode() const { return model; }
    int getBoundParameter() const
    {
        SpinLock::ScopedLockType sl (lock);
        return parameterIdx;
    }

private:
    SpinLock lock;
    const int index;
    Atomic<float> value { 0.f };
    Node model;
    GraphNodePtr node;
    AudioProcessor* processor = nullptr;
    Element::Parameter::Ptr parameter = nullptr;
    int parameterIdx = -1;
    bool special = false;
    bool recursionBlock = false;
    SignalConnection removedConnection;
};

class ElementPluginAudioProcessor  : public AudioProcessor,
                                     private AsyncUpdater
{
public:    
    ElementPluginAudioProcessor();
    ~ElementPluginAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isNodeBoundToAnyPerformanceParameter (const Node& boundNode, int boundParam) const;
    PopupMenu getPerformanceParameterMenu (int perfParam);
    void handlePerformanceParameterResult (int result, int perfParam);

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect () const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void numChannelsChanged() override;
    void numBusesChanged() override;
    void processorLayoutsChanged() override;

    AppController* getAppController() const { return controller.get(); }
    
    void setEditorBounds (const Rectangle<int>& bounds) { editorBounds = bounds; }
    const Rectangle<int>& getEditorBounds() const { return editorBounds; }

    void updateLatencySamples();
    void updateUnlockStatus();

    bool getEditorWantsKeyboard() const { return editorWantsKeyboard; }
    void setEditorWantsKeyboard (bool wantsIt) { editorWantsKeyboard = wantsIt; }
    
    Signal<void()> onPerfParamsChanged;

private:
    Array<PerformanceParameter*> perfparams;
    struct PerfParamMenuItem
    {
        Node node;
        int parameter = -1;
        bool unlink = false;
    };

    OwnedArray<PerfParamMenuItem> menuMap;
    ScopedPointer<Globals> world;
    ScopedPointer<AppController> controller;
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
    
    friend class AsyncUpdater;
    void handleAsyncUpdate() override;
    void reloadEngine();
    void initialize();
    
    var hasCheckedLicense { 0 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ElementPluginAudioProcessor)
};

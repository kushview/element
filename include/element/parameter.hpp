// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <boost/signals2.hpp>

#include <element/juce/core.hpp>
#include <element/porttype.hpp>

#pragma once

namespace element {

/** An abstract base class for parameter objects that can be added to a Node.

    Based on juce::AudioProcessorParameter, but designed for GraphNodes which 
    can change parameters.
*/
class Parameter : public juce::ReferenceCountedObject {
public:
    /** Contructor */
    Parameter() noexcept;

    /** Destructor. */
    virtual ~Parameter();

    static int defaultNumSteps() { return 0x7fffffff; }

    /** Returns the port index of this parameter */
    virtual int getPortIndex() const noexcept = 0;

    /** Returns the index of this parameter in its parent nodes's parameter list. */
    virtual int getParameterIndex() const noexcept = 0;

    /** Called by the host to find out the value of this parameter.

        Hosts will expect the value returned to be between 0 and 1.0.

        This could be called quite frequently, so try to make your code efficient.
        It's also likely to be called by non-UI threads, so the code in here should
        be thread-aware.
    */
    virtual float getValue() const = 0;

    /** The host will call this method to change the value of a parameter.

        The host may call this at any time, including during the audio processing
        callback, so your implementation has to process this very efficiently and
        avoid any kind of locking.

        If you want to set the value of a parameter internally, e.g. from your
        editor component, then don't call this directly - instead, use the
        setValueNotifyingHost() method, which will also send a message to
        the host telling it about the change. If the message isn't sent, the host
        won't be able to automate your parameters properly.

        The value passed will be between 0 and 1.0.
    */
    virtual void setValue (float newValue) = 0;

    /** This should return the default value for this parameter. */
    virtual float getDefaultValue() const = 0;

    /** Should parse a string and return the appropriate value for it. */
    virtual float getValueForText (const juce::String& text) const = 0;

    /** Returns the name to display for this parameter, which should be made
        to fit within the given string length.
    */
    virtual juce::String getName (int maximumStringLength) const = 0;

    /** Some parameters may be able to return a label string for
        their units. For example "Hz" or "%".
    */
    virtual juce::String getLabel() const = 0;

    /** Returns the number of steps that this parameter's range should be quantised into.

        If you want a continuous range of values, don't override this method, and allow
        the default implementation to return AudioProcessor::getDefaultNumParameterSteps().

        If your parameter is boolean, then you may want to make this return 2.

        The value that is returned may or may not be used, depending on the host. If you
        want the host to display stepped automation values, rather than a continuous
        interpolation between successive values, you should override isDiscrete to return true.

        @see isDiscrete
    */
    virtual int getNumSteps() const;

    /** Returns whether the parameter uses discrete values, based on the result of
        getNumSteps, or allows the host to select values continuously.

        This information may or may not be used, depending on the host. If you
        want the host to display stepped automation values, rather than a continuous
        interpolation between successive values, override this method to return true.

        @see getNumSteps
    */
    virtual bool isDiscrete() const;

    /** Returns whether the parameter represents a boolean switch, typically with
        "On" and "Off" states.

        This information may or may not be used, depending on the host. If you
        want the host to display a switch, rather than a two item dropdown menu,
        override this method to return true. You also need to override
        isDiscrete() to return `true` and getNumSteps() to return `2`.

        @see isDiscrete getNumSteps
    */
    virtual bool isBoolean() const;

    /** Returns a textual version of the supplied normalised parameter value.
        The default implementation just returns the floating point value
        as a string, but this could do anything you need for a custom type
        of value.
    */
    virtual juce::String getText (float normalisedValue, int /*maximumStringLength*/) const;

    /** This can be overridden to tell the host that this parameter operates in the
        reverse direction.
        (Not all plugin formats or hosts will actually use this information).
    */
    virtual bool isOrientationInverted() const;

    /** Returns true if the host can automate this parameter.
        By default, this returns true.
    */
    virtual bool isAutomatable() const;

    /** Should return true if this parameter is a "meta" parameter.
        A meta-parameter is a parameter that changes other params. It is used
        by some hosts (e.g. AudioUnit hosts).
        By default this returns false.
    */
    virtual bool isMetaParameter() const;

    enum Category {
        genericParameter = (0 << 16) | 0, /** If your parameter is not a meter then you should use this category */

        inputGain = (1 << 16) | 0, /** Currently not used */
        outputGain = (1 << 16) | 1,

        /** The following categories tell the host that this parameter is a meter level value
            and therefore read-only. Most hosts will display these type of parameters as
            a meter in the generic view of your plug-in. Pro-Tools will also show the meter
            in the mixer view.
        */
        inputMeter = (2 << 16) | 0,
        outputMeter = (2 << 16) | 1,
        compressorLimiterGainReductionMeter = (2 << 16) | 2,
        expanderGateGainReductionMeter = (2 << 16) | 3,
        analysisMeter = (2 << 16) | 4,
        otherMeter = (2 << 16) | 5
    };

    /** Returns the parameter's category. */
    virtual Category getCategory() const;

    /** A processor should call this when it needs to change one of its parameters.

        This could happen when the editor or some other internal operation changes
        a parameter. This method will call the setValue() method to change the
        value, and will then send a message to the host telling it about the change.

        Note that to make sure the host correctly handles automation, you should call
        the beginChangeGesture() and endChangeGesture() methods to tell the host when
        the user has started and stopped changing the parameter.
    */
    void setValueNotifyingHost (float newValue);

    /** Sends a signal to the host to tell it that the user is about to start changing this
        parameter.
        This allows the host to know when a parameter is actively being held by the user, and
        it may use this information to help it record automation.
        If you call this, it must be matched by a later call to endChangeGesture().
    */
    void beginChangeGesture();

    /** Tells the host that the user has finished changing this parameter.
        This allows the host to know when a parameter is actively being held by the user,
        and it may use this information to help it record automation.
        A call to this method must follow a call to beginChangeGesture().
    */
    void endChangeGesture();

    //==============================================================================
    /** Returns the current value of the parameter as a juce::String.

        This function can be called when you are hosting plug-ins to get a
        more specialsed textual represenation of the current value from the
        plug-in, for example "On" rather than "1.0".

        If you are implementing a plug-in then you should ignore this function
        and instead override getText.
    */
    virtual juce::String getCurrentValueAsText() const;

    /** Returns the set of strings which represent the possible states a parameter
        can be in.

        If you are hosting a plug-in you can use the result of this function to
        populate a ComboBox listing the allowed values.

        If you are implementing a plug-in then you do not need to override this.
    */
    virtual juce::StringArray getValueStrings() const;

    //==============================================================================
    /**
        A base class for listeners that want to know about changes to an
        Parameter.

        Use Parameter::addListener() to register your listener with
        an Parameter.

        This Listener replaces most of the functionality in the
        AudioProcessorListener class, which will be deprecated and removed.
    */
    class Listener {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** Receives a callback when a parameter has been changed.

            IMPORTANT NOTE: This will be called synchronously when a parameter changes, and
            many audio processors will change their parameter during their audio callback.
            This means that not only has your handler code got to be completely thread-safe,
            but it's also got to be VERY fast, and avoid blocking. If you need to handle
            this event on your message thread, use this callback to trigger an AsyncUpdater
            or ChangeBroadcaster which you can respond to on the message thread.
        */
        virtual void controlValueChanged (int index, float value) = 0;

        /** Indicates that a parameter change gesture has started.

            E.g. if the user is dragging a slider, this would be called with gestureIsStarting
            being true when they first press the mouse button, and it will be called again with
            gestureIsStarting being false when they release it.

            IMPORTANT NOTE: This will be called synchronously, and many audio processors will
            call it during their audio callback. This means that not only has your handler code
            got to be completely thread-safe, but it's also got to be VERY fast, and avoid
            blocking. If you need to handle this event on your message thread, use this callback
            to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
            message thread.
        */
        virtual void controlTouched (int index, bool grabbed) = 0;
    };

    /** Registers a listener to receive events when the parameter's state changes.
        If the listener is already registered, this will not register it again.

        @see removeListener
    */
    void addListener (Listener* newListener);

    /** Removes a previously registered parameter listener

        @see addListener
    */
    void removeListener (Listener* listener);

    //==============================================================================
    /** @internal */
    void sendValueChangedMessageToListeners (float newValue);
    /** @internal */
    void sendGestureChangedMessageToListeners (bool touched);

private:
    friend class Processor;

    //==============================================================================
    int parameterIndex = -1;
    juce::CriticalSection listenerLock;
    juce::Array<Listener*> listeners;
    mutable juce::StringArray valueStrings;

#if JUCE_DEBUG
    bool isPerformingGesture = false;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Parameter)
};

using ParameterPtr = juce::ReferenceCountedObjectPtr<Parameter>;
using ParameterArray = juce::ReferenceCountedArray<Parameter>;

/** A parameter type which represents a Control port. */
class RangedParameter : public Parameter {
public:
    RangedParameter (const PortDescription&);
    ~RangedParameter();

    /** Returns the human understandable value. */
    float get() const { return value; }
    operator float() const { return value; }

    /** Sets the value.
        @param newValue Human understandable value.
    */
    void set (float newValue) { operator= (newValue); }
    RangedParameter& operator= (float newValue);

    /** Returns the index of this port. */
    int getPortIndex() const noexcept override { return port.index; }

    /** Returns the index of this parameter as stored by the parent Processor. */
    int getParameterIndex() const noexcept override { return port.channel; }

    /** Returns category for this control */
    Category getCategory() const override { return Parameter::genericParameter; }

    /** Returns the ports value ranged from 0 to 1 */
    float getValue() const override { return range.convertTo0to1 (value); }

    /** Set the value in a.
        @param newValue Must be ranged 0 to 1
    */
    void setValue (float newValue) override { value = range.convertFrom0to1 (newValue); }

    /** Returns the value in a 0 to 1 range */
    float getDefaultValue() const override { return range.convertTo0to1 (port.defaultValue); }

    /** Returns the name of this Control port */
    juce::String getName (int maxLength) const override { return port.name.substring (0, maxLength); }

    /** Gets a label that should be used for this port. */
    juce::String getLabel() const override { return {}; }

    /** Get a text representation of this port. */
    juce::String getText (float normalisedValue, int maxLength) const override;

    /** Tries to get the 0 to 1 value of this port. */
    float getValueForText (const juce::String& text) const override { return convertTo0to1 (text.getFloatValue()); }

    /** Change port metadata. */
    void setPort (const PortDescription& newPort, bool preserveValue = false);

    /** Get port information. */
    PortDescription getPort() const { return port; }

    /** Get the cooresponding channel to the port index. */
    int getPortChannel() const { return port.channel; }

    /** Translate a human value to a 0 to 1 ranged value */
    float convertTo0to1 (float input) const { return range.convertTo0to1 (input); }

    /** Translates a 0 to 1 value to a human value */
    float convertFrom0to1 (float input) const { return range.convertFrom0to1 (input); }

    /** Get the range used by this port */
    const juce::NormalisableRange<float>& getNormalisableRange() const { return range; }

private:
    PortDescription port;
    juce::NormalisableRange<float> range;
    float value { 0.0 };
};

using RangedParameterPtr = juce::ReferenceCountedObjectPtr<RangedParameter>;

//==============================================================================
/** Class to aid in handling changes in a parameter.
    Currently just a placeholder until API can be created.
*/
class PortObserver {
public:
    PortObserver() = default;
    virtual ~PortObserver() = default;
};

//==============================================================================
class ParameterObserver : private PortObserver,
                          private Parameter::Listener,
                          private juce::Timer {
public:
    ParameterObserver() = default;
    ParameterObserver (ParameterPtr param)
        : parameter (param)
    {
        observeParameter (param);
    }

    ~ParameterObserver() override
    {
        stopTimer();
        if (parameter != nullptr) {
            parameter->removeListener (this);
            parameter = nullptr;
        }
    }

    void observeParameter (ParameterPtr param)
    {
        if (parameter == param)
            return;
        if (parameter) {
            parameter->removeListener (this);
        }
        parameter = param;
        if (parameter != nullptr) {
            parameter->addListener (this);
            startTimer (100);
        } else {
            stopTimer();
        }
    }

    /** Returns the parameter being observed. */
    ParameterPtr getParameter() noexcept { return parameter; }

    /** Invoked on the UI thread when the parameter changes. */
    boost::signals2::signal<void()> sigValueChanged;

protected:
    /** Called on the UI thread when the parameter value changes */
    virtual void handleNewParameterValue() {}

private:
    //==============================================================================
    void controlValueChanged (int, float) override
    {
        parameterValueHasChanged = 1;
    }

    void controlTouched (int, bool) override {}

    //==============================================================================
    void timerCallback() override
    {
        if (parameterValueHasChanged.compareAndSetBool (0, 1)) {
            handleNewParameterValue();
            sigValueChanged();
            startTimerHz (50);
        } else {
            startTimer (juce::jmin (250, getTimerInterval() + 10));
        }
    }

    ParameterPtr parameter;
    juce::Atomic<int> parameterValueHasChanged { 0 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterObserver)
};

} // namespace element

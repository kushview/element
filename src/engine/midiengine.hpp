// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ElementApp.h" // FIXME:

#pragma once

namespace element {

class Settings;

class MidiEngine : public juce::ChangeBroadcaster
{
public:
    MidiEngine();
    ~MidiEngine();

    //==============================================================================
    void applySettings (Settings&);
    void writeSettings (Settings&);

    //==============================================================================
    /** Enables or disables a midi input device.

        The list of devices can be obtained with the MidiInput::getAvailableDevices() method.

        Any incoming messages from enabled input devices will be forwarded on to all the
        listeners that have been registered with the addMidiInputCallback() method. They
        can either register for messages from a particular device, or from just the
        "default" midi input.

        Routing the midi input via an AudioDeviceManager means that when a listener
        registers for the default midi input, this default device can be changed by the
        manager without the listeners having to know about it or re-register.

        It also means that a listener can stay registered for a midi input that is disabled
        or not present, so that when the input is re-enabled, the listener will start
        receiving messages again.

        @see addMidiInputCallback, isMidiInputEnabled
    */
    void setMidiInputEnabled (const MidiDeviceInfo& deviceId, bool enabled);

    /** Returns true if a given midi input device is being used.
        @see setMidiInputEnabled
    */
    bool isMidiInputEnabled (const MidiDeviceInfo& deviceId) const;

    /** Registers a listener for callbacks when midi events arrive from a midi input.

        The device name can be empty to indicate that it wants to receive all incoming
        events from all the enabled MIDI inputs. Or it can be the name of one of the
        MIDI input devices if it just wants the events from that device. (see
        MidiInput::getAvailableDevices() for the list of device names).

        Only devices which are enabled (see the setMidiInputEnabled() method) will have their
        events forwarded on to listeners.
     */
    void addMidiInputCallback (const MidiDeviceInfo& device, MidiInputCallback* callback, bool consumer = false);
    void addMidiInputCallback (const String& identifier, MidiInputCallback* callback, bool consumer = false);
    void addMidiInputCallback (MidiInputCallback* callback, bool consumer = false);

    /** Removes a listener that was previously registered with addMidiInputCallback(). */
    void removeMidiInputCallback (const MidiDeviceInfo& device, MidiInputCallback* callback);

    /** Removes a listener that was previously registered with addMidiInputCallback().
        This version does not check device name.
     */
    void removeMidiInputCallback (MidiInputCallback* callback);

    /** Returns the number of enabled midi inputs */
    int getNumActiveMidiInputs() const;

    //==============================================================================
    /** Sets a midi output device to use as the default.

        The list of devices can be obtained with the MidiOutput::getAvailableDevices() method.

        The specified device will be opened automatically and can be retrieved with the
        getDefaultMidiOutput() method.

        Pass in an empty string to deselect all devices. For the default device, you
        can use MidiOutput::getAvailableDevices() [MidiOutput::getDefaultDeviceIndex()].

        @see getDefaultMidiOutput, getDefaultMidiOutputName
    */
    void setDefaultMidiOutput (const MidiDeviceInfo& device);

    /** Returns the name of the default midi output.
        @see setDefaultMidiOutput, getDefaultMidiOutput
     */
    const String& getDefaultMidiOutputName() const noexcept { return defaultMidiOutputName; }

    const String& getDefaultMidiOutputID() const noexcept { return defaultMidiOutputName; }

    /** Returns the current default midi output device.
        If no device has been selected, or the device can't be opened, this will return nullptr.
        @see getDefaultMidiOutputName
    */
    MidiOutput* getDefaultMidiOutput() const noexcept { return defaultMidiOutput.get(); }

    void processMidiBuffer (const MidiBuffer& buffer, int nframes, double sampleRate);

    CriticalSection& getMidiOutputLock() { return midiOutputLock; }

private:
    struct MidiCallbackInfo
    {
        String device;

        /** If true, will receive callbacks no matter the active state for the
            Audio Engine. If false, then will only receive callbacks if active
            for the audio engine.
         */
        bool consumer = false;

        MidiInputCallback* callback { nullptr };
    };

    struct MidiInputHolder : public MidiInputCallback
    {
        MidiInputHolder (MidiEngine& e)
            : engine (e) {}

        std::unique_ptr<MidiInput> input;
        bool active = false; // if true, then will feed to audio engine

        void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;

    private:
        MidiEngine& engine;
    };

    StringArray midiInsFromXml;
    OwnedArray<MidiInputHolder> openMidiInputs;
    Array<MidiCallbackInfo> midiCallbacks;

    String defaultMidiOutputName, defaultMidiOutputID;
    std::unique_ptr<MidiOutput> defaultMidiOutput;
    CriticalSection audioCallbackLock, midiCallbackLock, midiOutputLock;

    class CallbackHandler;
    std::unique_ptr<CallbackHandler> callbackHandler;

    MidiInputHolder* getMidiInput (const String& identifier, bool openIfNotAlready);
    void handleIncomingMidiMessageInt (juce::MidiInput*, const juce::MidiMessage&);
};

} // namespace element

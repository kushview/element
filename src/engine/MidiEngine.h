
#include "JuceHeader.h"

#pragma once

namespace Element {

class Settings;

class MidiEngine : public ChangeBroadcaster
{
public:
    MidiEngine();
    ~MidiEngine();

    //==============================================================================
    void applySettings (Settings&);
    void writeSettings (Settings&);

    //==============================================================================
    /** Enables or disables a midi input device.

        The list of devices can be obtained with the MidiInput::getDevices() method.

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
    void setMidiInputEnabled (const String& midiInputDeviceName, bool enabled);

    /** Returns true if a given midi input device is being used.
        @see setMidiInputEnabled
    */
    bool isMidiInputEnabled (const String& midiInputDeviceName) const;

    /** Registers a listener for callbacks when midi events arrive from a midi input.

        The device name can be empty to indicate that it wants to receive all incoming
        events from all the enabled MIDI inputs. Or it can be the name of one of the
        MIDI input devices if it just wants the events from that device. (see
        MidiInput::getDevices() for the list of device names).

        Only devices which are enabled (see the setMidiInputEnabled() method) will have their
        events forwarded on to listeners.
     */
    void addMidiInputCallback (const String& midiInputDeviceName,
                               MidiInputCallback* callback);

    /** Removes a listener that was previously registered with addMidiInputCallback(). */
    void removeMidiInputCallback (const String& midiInputDeviceName,
                                  MidiInputCallback* callback);

    //==============================================================================
    /** Sets a midi output device to use as the default.

        The list of devices can be obtained with the MidiOutput::getDevices() method.

        The specified device will be opened automatically and can be retrieved with the
        getDefaultMidiOutput() method.

        Pass in an empty string to deselect all devices. For the default device, you
        can use MidiOutput::getDevices() [MidiOutput::getDefaultDeviceIndex()].

        @see getDefaultMidiOutput, getDefaultMidiOutputName
    */
    void setDefaultMidiOutput (const String& deviceName);

    /** Returns the name of the default midi output.
        @see setDefaultMidiOutput, getDefaultMidiOutput
     */
    const String& getDefaultMidiOutputName() const noexcept         { return defaultMidiOutputName; }

    /** Returns the current default midi output device.
        If no device has been selected, or the device can't be opened, this will return nullptr.
        @see getDefaultMidiOutputName
    */
    MidiOutput* getDefaultMidiOutput() const noexcept               { return defaultMidiOutput.get(); }

    CriticalSection& getMidiOutputLock() { return midiCallbackLock; }

private:
    struct MidiCallbackInfo
    {
        String deviceName;
        MidiInputCallback* callback;
    };

    StringArray midiInsFromXml;
    OwnedArray<MidiInput> enabledMidiInputs;
    Array<MidiCallbackInfo> midiCallbacks;

    String defaultMidiOutputName;
    std::unique_ptr<MidiOutput> defaultMidiOutput;
    CriticalSection audioCallbackLock, midiCallbackLock;

    class CallbackHandler;
    std::unique_ptr<CallbackHandler> callbackHandler;

    void handleIncomingMidiMessageInt (MidiInput*, const MidiMessage&);
};

}

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "nodes/mididevice.hpp"
#include "engine/midiengine.hpp"
#include <element/ui/style.hpp>

namespace element {

class MidiDeviceEditor : public AudioProcessorEditor,
                         public ComboBox::Listener,
                         public Button::Listener,
                         public Timer
{
public:
    MidiDeviceEditor (MidiDeviceProcessor& p, const bool isInput)
        : AudioProcessorEditor (&p), proc (p), inputDevice (isInput)
    {
        setOpaque (true);
        addAndMakeVisible (deviceBox);
        deviceBox.addListener (this);

        addAndMakeVisible (statusButton);
        statusButton.setColour (TextButton::buttonColourId, Colors::toggleRed);
        statusButton.setColour (TextButton::buttonOnColourId, Colors::toggleGreen);
        statusButton.setToggleState (false, dontSendNotification);
        statusButton.addListener (this);

        if (! isInput)
        {
            addAndMakeVisible (midiOutLatencyLabel);
            midiOutLatencyLabel.setText ("Output latency (ms)", dontSendNotification);
            midiOutLatencyLabel.setFont (Font (FontOptions (12.f)));
            addAndMakeVisible (midiOutLatency);
            midiOutLatency.setRange (-1000.0, 1000.0, 1.0);
            midiOutLatency.setValue (proc.getLatency(), dontSendNotification);
            midiOutLatency.textFromValueFunction = [] (double value) -> juce::String { return String (roundToInt (value)) + " ms"; };
            midiOutLatency.onValueChange = [this]() { proc.setLatency (midiOutLatency.getValue()); };
            midiOutLatency.updateText();

            setSize (240, 120);
        }
        else
        {
            setSize (240, 80);
        }

        startTimer (1000 * 2.5);
    }

    ~MidiDeviceEditor()
    {
        stopTimer();
        deviceBox.removeListener (this);
    }

    void stabilizeComponents()
    {
        statusButton.setToggleState (proc.isDeviceOpen(), dontSendNotification);
        if (! inputDevice)
        {
            midiOutLatency.setValue (proc.getLatency(), dontSendNotification);
        }
    }

    void buttonClicked (Button*) override
    {
        proc.reload();
        stabilizeComponents();
    }

    void timerCallback() override
    {
        stabilizeComponents();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (element::Colors::backgroundColor);
    }

    void resized() override
    {
        const int widgetSize = 18;
        auto r = getLocalBounds().withSizeKeepingCentre (180, widgetSize);
        deviceBox.setBounds (r.withLeft (r.getX() + 4 + widgetSize / 2));
        statusButton.setBounds (deviceBox.getX() - widgetSize - 4, deviceBox.getY(), widgetSize, widgetSize);

        if (! inputDevice)
        {
            r = getLocalBounds();
            midiOutLatency.setBounds (r.removeFromBottom (widgetSize));
            midiOutLatencyLabel.setBounds (r.removeFromBottom (widgetSize));
        }
    }

    void comboBoxChanged (ComboBox*) override
    {
        auto device = devices[deviceBox.getSelectedItemIndex()];
        proc.setDevice (device);
        stabilizeComponents();
    }

private:
    friend class MidiDeviceProcessor;

    MidiDeviceProcessor& proc;
    const bool inputDevice;
    Array<MidiDeviceInfo> devices;
    int notAvailableDeviceId = 0;
    MidiDeviceInfo notAvailableDev;
    bool hasNotAvailableDevice() const { return notAvailableDeviceId > 0; }

    ComboBox deviceBox;
    TextButton statusButton;
    Slider midiOutLatency;
    Label midiOutLatencyLabel;

    void updateDevices (const bool resetList = true)
    {
        notAvailableDeviceId = 0;
        notAvailableDev = {};

        if (resetList)
            devices = proc.getAvailableDevices();
        deviceBox.clear (dontSendNotification);
        for (int i = 0; i < devices.size(); ++i)
            deviceBox.addItem (devices[i].name, i + 1);

        if (! proc.isDeviceOpen())
        {
            notAvailableDev = proc.getWantedDevice();
            if (notAvailableDev.identifier.isNotEmpty())
            {
                notAvailableDeviceId = 1000;
                deviceBox.addItem (notAvailableDev.name, notAvailableDeviceId);
            }
        }
        else
        {
            deviceBox.setSelectedItemIndex (devices.indexOf (proc.getDevice()));
        }
    }
};

MidiDeviceProcessor::MidiDeviceProcessor (const bool isInput, MidiEngine& me)
    : BaseProcessor(),
      inputDevice (isInput),
      midi (me)
{
    setPlayConfigDetails (0, 0, 44100.0, 1024);
}

MidiDeviceProcessor::~MidiDeviceProcessor() noexcept
{
    closeDevice();
}

void MidiDeviceProcessor::setLatency (double latencyMs)
{
    if (inputDevice)
        return;
    midiOutLatency.set (jlimit (-1000.0, 1000.0, latencyMs));
}

void MidiDeviceProcessor::setDevice (const MidiDeviceInfo& newDevice)
{
    if (device.identifier == newDevice.identifier
        && deviceWanted.identifier == newDevice.identifier
        && isDeviceOpen())
    {
        DBG ("[element] returned setting new device");
        return;
    }

    if (newDevice.identifier.isEmpty())
    {
        closeDevice();
        DBG ("[element] closed on call to set");
        return;
    }

    deviceWanted = newDevice;

    // check if device online
    const bool wasSuspended = isSuspended();
    suspendProcessing (true);
    const bool wasPrepared = prepared;
    const double rate = getSampleRate();
    const int block = getBlockSize();

    if (prepared)
        releaseResources();

    if (inputDevice)
    {
        midi.removeMidiInputCallback (this);
        if (deviceWanted.identifier.isNotEmpty())
        {
            midi.addMidiInputCallback (deviceWanted, this, true);
            device = deviceWanted;
        }
    }
    else
    {
        if (output)
        {
            output->stopBackgroundThread();
            output->clearAllPendingMessages();
            output.reset();
        }

        output = MidiOutput::openDevice (deviceWanted.identifier);

        if (output)
        {
            output->clearAllPendingMessages();
            output->startBackgroundThread();
            device = deviceWanted;
        }
        else
        {
            DBG ("[element] could not open MIDI output: " << deviceWanted.name);
        }
    }

    if (! isDeviceOpen())
    {
        startTimer (1500);
    }
    else
    {
        DBG ("[element] midi device opened " << device.name);
    }

    if (wasPrepared)
        prepareToPlay (rate, block);

    suspendProcessing (wasSuspended);
}

Result MidiDeviceProcessor::closeDevice()
{
    const bool wasSuspended = isSuspended();
    suspendProcessing (true);

    if (inputDevice)
    {
        midi.removeMidiInputCallback (this);
    }
    else
    {
        if (output)
        {
            output->clearAllPendingMessages();
            std::unique_ptr<MidiOutput> closer;
            {
                ScopedLock sl (getCallbackLock());
                std::swap (output, closer);
            }
            closer.reset();
        }
    }

    suspendProcessing (wasSuspended);
    device.identifier.clear();
    device.name.clear();
    return Result::ok();
}

bool MidiDeviceProcessor::isDeviceOpen() const
{
    if (inputDevice)
    {
        return device.identifier.isNotEmpty() && deviceWanted.identifier == device.identifier;
    }

    ScopedLock sl (getCallbackLock());
    return output != nullptr;
}

void MidiDeviceProcessor::reload()
{
    // noop
}

String MidiDeviceProcessor::getDeviceName() const noexcept
{
    return device.name;
}

void MidiDeviceProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    inputMessages.reset (sampleRate);
    if (prepared)
        return;

    if (! isDeviceOpen())
    {
        if (deviceWanted.identifier.isNotEmpty())
            if (deviceIsAvailable (deviceWanted))
                setDevice (deviceWanted);
    }

    setPlayConfigDetails (0, 0, sampleRate, maximumExpectedSamplesPerBlock);
    prepared = true;
}

void MidiDeviceProcessor::processBlock (AudioBuffer<float>& audio, MidiBuffer& midi)
{
    const auto nframes = audio.getNumSamples();
    if (inputDevice)
    {
        midi.clear (0, nframes);
        inputMessages.removeNextBlockOfMessages (midi, nframes);
    }
    else
    {
        if (output && ! midi.isEmpty())
        {
            const auto delayMs = midiOutLatency.get();
#if JUCE_WINDOWS
            output->sendBlockOfMessagesNow (midi);
#else
            output->sendBlockOfMessages (
                midi, delayMs + Time::getMillisecondCounterHiRes(), getSampleRate());
#endif
        }

        midi.clear (0, nframes);
    }
}

void MidiDeviceProcessor::releaseResources()
{
    prepared = false;
    inputMessages.reset (getSampleRate());
}

AudioProcessorEditor* MidiDeviceProcessor::createEditor()
{
    auto* const editor = new MidiDeviceEditor (*this, inputDevice);
    editor->updateDevices (true);
    return editor;
}

void MidiDeviceProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    ValueTree state ("state");
    state.setProperty ("inputDevice", isInputDevice(), 0)
        .setProperty ("deviceName", deviceWanted.name, 0)
        .setProperty (tags::identifier, deviceWanted.identifier, nullptr)
        .setProperty ("midiLatency", midiOutLatency.get(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MidiDeviceProcessor::setStateInformation (const void* data, int size)
{
    ValueTree state;
    if (auto xml = getXmlFromBinary (data, size))
        state = ValueTree::fromXml (*xml);
    if (! state.isValid())
        return;
    midiOutLatency.set (state.getProperty ("midiLatency", (double) midiOutLatency.get()));
    if (inputDevice != (bool) state.getProperty ("inputDevice"))
    {
        DBG ("[element] MIDI Device node wrong direction");
    }

    MidiDeviceInfo info;
    info.name = state.getProperty ("deviceName", "").toString();
    info.identifier = state.getProperty (tags::identifier).toString();
    DBG ("[element] restoring device state: " << info.name);
    DBG ("[element] MIDI Device ID: " << info.identifier);
    closeDevice();
    setDevice (info);
}

void MidiDeviceProcessor::handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
{
    if (message.isActiveSense())
        return;
    inputMessages.addMessageToQueue (message);
}

void MidiDeviceProcessor::handlePartialSysexMessage (MidiInput* source, const uint8* messageData, int numBytesSoFar, double timestamp)
{
    ignoreUnused (source, messageData, numBytesSoFar, timestamp);
}

Array<MidiDeviceInfo> MidiDeviceProcessor::getAvailableDevices() const noexcept
{
    const auto devlist = input ? MidiInput::getAvailableDevices()
                               : MidiOutput::getAvailableDevices();
    return devlist;
}

bool MidiDeviceProcessor::deviceIsAvailable (const String& name)
{
    for (const auto& info : getAvailableDevices())
    {
        if (info.name == name)
            return true;
    }
    return true;
}

bool MidiDeviceProcessor::deviceIsAvailable (const MidiDeviceInfo& dev)
{
    for (const auto& info : getAvailableDevices())
        if (info == dev)
            return true;
    return true;
}

void MidiDeviceProcessor::timerCallback()
{
    if (deviceWanted.identifier.isEmpty())
    {
        stopTimer();
        return;
    }
    DBG ("waiting for wanted device: " << deviceWanted.name);
    setDevice (deviceWanted);
}

} // namespace element

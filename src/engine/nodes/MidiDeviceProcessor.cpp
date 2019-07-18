
#include "engine/nodes/MidiDeviceProcessor.h"
#include "engine/MidiEngine.h"
#include "gui/LookAndFeel.h"

namespace Element {

class MidiDeviceEditor : public AudioProcessorEditor,
                         public ComboBox::Listener,
                         public Button::Listener,
                         public Timer
{
public:
    MidiDeviceEditor (MidiDeviceProcessor& p, const bool isInput)
        : AudioProcessorEditor (&p), proc (p), inputDevice (isInput)
    {
        setOpaque (false);
        addAndMakeVisible (deviceBox);
        deviceBox.addListener (this);

        addAndMakeVisible (statusButton);
        statusButton.setColour (TextButton::buttonColourId, Colors::toggleRed);
        statusButton.setColour (TextButton::buttonOnColourId, Colors::toggleGreen);
        statusButton.setToggleState (false, dontSendNotification);
        statusButton.addListener (this);

        setSize (240, 80);

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
        g.fillAll (Element::LookAndFeel::backgroundColor);
    }

    void resized() override
    {
        const int widgetSize = 18;
        const auto r = getLocalBounds().withSizeKeepingCentre (180, widgetSize);
        deviceBox.setBounds (r.withLeft (r.getX() + 4 + widgetSize / 2));
        statusButton.setBounds (deviceBox.getX() - widgetSize - 4, deviceBox.getY(), 
                                widgetSize ,widgetSize);
    }

    void comboBoxChanged (ComboBox*) override
    {
        const auto deviceName = deviceBox.getItemText (deviceBox.getSelectedItemIndex());
        proc.setCurrentDevice (deviceName);
        stabilizeComponents();
    }

private:
    friend class MidiDeviceProcessor;

    MidiDeviceProcessor& proc;
    const bool inputDevice;
    StringArray devices;
    ComboBox deviceBox;
    TextButton statusButton;

    void updateDevices (const bool resetList = true)
    {
        if (resetList)
            devices = inputDevice ? MidiInput::getDevices() : MidiOutput::getDevices();
        deviceBox.clear (dontSendNotification);
        for (int i = 0; i < devices.size(); ++i)
            deviceBox.addItem (devices [i], i + 1);
        deviceBox.setSelectedItemIndex (devices.indexOf (proc.getName()));
    }
};

MidiDeviceProcessor::MidiDeviceProcessor (const bool isInput, MidiEngine& me)
    : BaseProcessor(),
      inputDevice (isInput),
      midi (me)
{
    setPlayConfigDetails (0, 0, 44100.0, 1024);
}

MidiDeviceProcessor::~MidiDeviceProcessor() noexcept { }

void MidiDeviceProcessor::setCurrentDevice (const String& device)
{
    const bool wasSuspended = isSuspended();
    suspendProcessing (true);
    const bool wasPrepared = prepared;
    const double rate = getSampleRate();
    const int block = getBlockSize();

    if (prepared)
        releaseResources();

    deviceName = device;

    if (wasPrepared)
        prepareToPlay (rate, block);

    suspendProcessing (wasSuspended);
}

bool MidiDeviceProcessor::isDeviceOpen() const
{
    ScopedLock sl (getCallbackLock());
    return inputDevice ? deviceName.isNotEmpty() : output != nullptr;
}

void MidiDeviceProcessor::reload()
{
    setCurrentDevice (deviceName);
}

const String MidiDeviceProcessor::getName() const
{
    return deviceName;
}

void MidiDeviceProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    inputMessages.reset (sampleRate);
    if (prepared)
        return;
    
    const StringArray devList = inputDevice ? MidiInput::getDevices() : MidiOutput::getDevices();
    const int defaultIdx = inputDevice ? MidiInput::getDefaultDeviceIndex() : MidiOutput::getDefaultDeviceIndex();
    int deviceIdx = deviceName.isNotEmpty() ? devList.indexOf (deviceName) : defaultIdx;
    if (deviceIdx < 0) deviceIdx = defaultIdx;

    if (inputDevice)
    {
        if (deviceName.isNotEmpty())
            midi.addMidiInputCallback (deviceName, this, true);
    }
    else
    {
        output = MidiOutput::openDevice (deviceIdx);
        if (output)
        {
            output->clearAllPendingMessages();
            output->startBackgroundThread();
        } 
        else 
        {
            DBG("[EL] could not open MIDI output: " << deviceIdx << ": " << deviceName);
        }
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
        if (output && !midi.isEmpty())
        {
            const double delayMs = 6.0;
            output->sendBlockOfMessages (
                midi, delayMs + Time::getMillisecondCounterHiRes(), getSampleRate());
        }

        midi.clear (0, nframes);
    }
}

void MidiDeviceProcessor::releaseResources()
{
    prepared = false;
    inputMessages.reset (getSampleRate());
    midi.removeMidiInputCallback (this);

    if (input)
    {
        input->stop();
        input = nullptr;
    }

    if (output)
    {
        output->stopBackgroundThread();
        output = nullptr;
    }
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
         .setProperty ("deviceName", deviceName, 0);
    if (ScopedPointer<XmlElement> xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MidiDeviceProcessor::setStateInformation (const void* data, int size)
{
    ValueTree state;
    if (ScopedPointer<XmlElement> xml = getXmlFromBinary (data, size))
        state = ValueTree::fromXml (*xml);
    if (! state.isValid())
        return;
    if (inputDevice != (bool) state.getProperty ("inputDevice"))
    {
        DBG("[EL] MIDI Device node wrong direction");
    }
    setCurrentDevice (state.getProperty ("deviceName", "").toString());
}

void MidiDeviceProcessor::handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
{
    if (message.isActiveSense())
        return;
    inputMessages.addMessageToQueue (message);
}

void MidiDeviceProcessor::handlePartialSysexMessage (MidiInput* source, const uint8* messageData,
                                                     int numBytesSoFar, double timestamp)
{
    ignoreUnused (source, messageData, numBytesSoFar, timestamp);
}
                                
}

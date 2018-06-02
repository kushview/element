
#include "engine/MidiDeviceProcessor.h"

namespace Element {

MidiDeviceProcessor::MidiDeviceProcessor (const bool isInput)
    : BaseProcessor(),
      inputDevice (isInput)
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

const String MidiDeviceProcessor::getName() const
{
    String name;
    if (isInputDevice() && input != nullptr)
        name = input->getName();
    if (isOutputDevice() && output != nullptr)
        name = output->getName();
    return name.isNotEmpty() ? name : "N/A";
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
        input = MidiInput::openDevice (deviceIdx, this);
        if (input) input->start();
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
            traceMidi (midi); DBG("samplerate: " << getSampleRate());
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
    return new GenericAudioProcessorEditor (this);
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
    setCurrentDevice (state.getProperty("deviceName", "").toString());
}

void MidiDeviceProcessor::handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
{
    if (input == nullptr || input != source)
        return;
    inputMessages.addMessageToQueue (message);
}

void MidiDeviceProcessor::handlePartialSysexMessage (MidiInput* source, const uint8* messageData,
                                                     int numBytesSoFar, double timestamp)
{
    ignoreUnused (source, messageData, numBytesSoFar, timestamp);
}
                                
}

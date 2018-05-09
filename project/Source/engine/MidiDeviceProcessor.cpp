
#include "engine/MidiDeviceProcessor.h"

namespace Element {

MidiDeviceProcessor::MidiDeviceProcessor (const bool isInput)
    : BaseProcessor(),
      inputDevice (isInput) 
{
}

MidiDeviceProcessor::~MidiDeviceProcessor() noexcept { }

const String MidiDeviceProcessor::getName() const
{
    if (isInputDevice() && input != nullptr)
        return input->getName();
    if (isOutputDevice() && output != nullptr)
        return output->getName();
    return "Unknown";
}

void MidiDeviceProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    if (inputDevice)
    {

    }
    else
    {

    }
}

void MidiDeviceProcessor::processBlock (AudioBuffer<float>&, MidiBuffer& midi)
{

}

void MidiDeviceProcessor::releaseResources()
{

}


AudioProcessorEditor* MidiDeviceProcessor::createEditor()
{
    return new GenericAudioProcessorEditor (this);
}

void MidiDeviceProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    ValueTree state ("state");
    state.setProperty ("inputDevice", isInputDevice(), 0)
         .setProperty ("deviceName", getName(), 0);
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
}

}

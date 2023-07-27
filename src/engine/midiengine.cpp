// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/juce/audio_basics.hpp>
#include <element/juce/audio_devices.hpp>
#include <element/juce/data_structures.hpp>

#include <element/settings.hpp>
#include <element/tags.hpp>

#include "engine/midiengine.hpp"

namespace element {
using juce::MidiInput;
using juce::MidiInputCallback;
using juce::MidiMessage;
using juce::ValueTree;
using juce::XmlElement;

//==============================================================================
void MidiEngine::applySettings (Settings& settings)
{
    midiInsFromXml.clear();

    if (auto xml = std::unique_ptr<XmlElement> (settings.getUserSettings()->getXmlValue (Settings::midiEngineKey)))
    {
        const auto data = ValueTree::fromXml (*xml);
        for (int i = 0; i < data.getNumChildren(); ++i)
        {
            const auto child (data.getChild (i));
            if (child.hasType (tags::input))
            {
                if (auto* const holder = getMidiInput (child[tags::identifier], true))
                {
                    holder->active = false; // open but not active initially
                    if ((bool) child[tags::active])
                        midiInsFromXml.add (child[tags::identifier]);
                }
            }
        }

        for (auto& m : MidiInput::getAvailableDevices())
            setMidiInputEnabled (m, midiInsFromXml.contains (m.identifier));

        MidiDeviceInfo info;
        info.name = data["defaultMidiOutput"].toString();
        info.identifier = data["defaultMidiOutputID"].toString();
        setDefaultMidiOutput (info);
    }
}

void MidiEngine::writeSettings (Settings& settings)
{
    ValueTree data ("MidiSettings");
    for (auto* const holder : openMidiInputs)
    {
        ValueTree input (tags::input);
        input.setProperty (tags::name, holder->input->getName(), nullptr)
            .setProperty (tags::identifier, holder->input->getIdentifier(), nullptr)
            .setProperty (tags::active, holder->active, nullptr);
        data.appendChild (input, nullptr);
    }

    if (midiInsFromXml.size() > 0)
    {
        // Add any midi devices that have been enabled before, but which aren't currently
        // open because the device has been disconnected.
        const auto avail = MidiInput::getAvailableDevices();

        StringArray availableIDs;
        for (const auto& amd : avail)
            availableIDs.add (amd.identifier);
        for (int i = 0; i < midiInsFromXml.size(); ++i)
        {
            if (availableIDs.contains (midiInsFromXml[i], false))
                continue;

            MidiDeviceInfo info;
            info.identifier = midiInsFromXml[i];
            for (const auto& amd2 : avail)
            {
                if (amd2.identifier == info.identifier)
                {
                    info.name = amd2.name;
                    break;
                }
            }
            ValueTree input (tags::input);
            input.setProperty (tags::name, info.name, nullptr)
                .setProperty (tags::identifier, info.identifier, nullptr)
                .setProperty (tags::active, true, nullptr);
            data.appendChild (input, nullptr);
        }
    }

    data.setProperty ("defaultMidiOutput", defaultMidiOutputName, nullptr);
    data.setProperty ("defaultMidiOutputID", defaultMidiOutputID, nullptr);

    if (auto xml = std::unique_ptr<XmlElement> (data.createXml()))
        settings.getUserSettings()->setValue (Settings::midiEngineKey, xml.get());
}

//==============================================================================
class MidiEngine::CallbackHandler : public juce::AudioIODeviceCallback,
                                    public juce::MidiInputCallback,
                                    public juce::AudioIODeviceType::Listener
{
public:
    CallbackHandler (MidiEngine& me) noexcept : owner (me) {}

private:
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override
    {
        ignoreUnused (inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples, context);
    }

    void audioDeviceAboutToStart (juce::AudioIODevice* device) override
    {
        ignoreUnused (device);
    }

    void audioDeviceStopped() override
    {
        // noop
    }

    void audioDeviceError (const String& message) override
    {
        // noop
    }

    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override
    {
        owner.handleIncomingMidiMessageInt (source, message);
    }

    void audioDeviceListChanged() override
    {
        // noop
    }

    MidiEngine& owner;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CallbackHandler)
};

void MidiEngine::MidiInputHolder::handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
{
    if (message.isActiveSense())
        return;

    jassert (source == input.get());
    const ScopedLock sl (engine.midiCallbackLock);

    for (auto& mc : engine.midiCallbacks)
        if ((active || mc.consumer) && (mc.device.isEmpty() || mc.device == input->getIdentifier()))
            mc.callback->handleIncomingMidiMessage (input.get(), message);
}

//==============================================================================
MidiEngine::MidiEngine()
{
    callbackHandler.reset (new CallbackHandler (*this));
}

MidiEngine::~MidiEngine()
{
    callbackHandler.reset (nullptr);
}

//==============================================================================
MidiEngine::MidiInputHolder* MidiEngine::getMidiInput (const String& identifier, bool openIfNotAlready)
{
    for (auto* const holder : openMidiInputs)
        if (holder->input && holder->input->getIdentifier() == identifier)
            return holder;

    if (! openIfNotAlready)
        return nullptr;

    int index = 0;
    bool found = false;
    for (const auto& dev : MidiInput::getAvailableDevices())
    {
        if (identifier != dev.identifier)
        {
            ++index;
            continue;
        }
        found = true;
        break;
    }

    if (found)
    {
        std::unique_ptr<MidiInputHolder> holder;
        holder.reset (new MidiInputHolder (*this));
        if (auto midiIn = MidiInput::openDevice (identifier, holder.get()))
        {
            holder->input.reset (midiIn.release());
            holder->input->start();
            return openMidiInputs.add (holder.release());
        }
    }

    return nullptr;
}

//==============================================================================
void MidiEngine::setMidiInputEnabled (const MidiDeviceInfo& device, const bool enabled)
{
    if (enabled != isMidiInputEnabled (device))
    {
        if (enabled)
        {
            if (auto* holder = getMidiInput (device.identifier, true))
                holder->active = true;
        }
        else
        {
            if (auto* holder = getMidiInput (device.identifier, false))
                holder->active = false;
        }

        sendChangeMessage();
    }
}

bool MidiEngine::isMidiInputEnabled (const MidiDeviceInfo& device) const
{
    for (auto* mi : openMidiInputs)
        if (mi->input != nullptr && mi->input->getIdentifier() == device.identifier && mi->active)
            return true;

    return false;
}

void MidiEngine::addMidiInputCallback (MidiInputCallback* callbackToAdd, bool consumer)
{
    addMidiInputCallback (MidiDeviceInfo(), callbackToAdd, consumer);
}

void MidiEngine::addMidiInputCallback (const String& identifier, MidiInputCallback* callback, bool consumer)
{
    MidiDeviceInfo info;
    info.identifier = identifier;
    addMidiInputCallback (info, callback, consumer);
}

void MidiEngine::addMidiInputCallback (const MidiDeviceInfo& device, MidiInputCallback* callbackToAdd, bool consumer)
{
    removeMidiInputCallback (device, callbackToAdd);

    if (device.identifier.isEmpty() || isMidiInputEnabled (device) || consumer)
    {
        if (consumer)
        {
            if (auto* holder = getMidiInput (device.identifier, true))
                ignoreUnused (holder);
        }

        MidiCallbackInfo mc;
        mc.device = device.identifier;
        mc.callback = callbackToAdd;
        mc.consumer = consumer;

        const ScopedLock sl (midiCallbackLock);
        midiCallbacks.add (mc);
    }
}

void MidiEngine::removeMidiInputCallback (const MidiDeviceInfo& device, MidiInputCallback* callbackToRemove)
{
    for (int i = midiCallbacks.size(); --i >= 0;)
    {
        auto& mc = midiCallbacks.getReference (i);

        if (mc.callback == callbackToRemove && mc.device == device.identifier)
        {
            const ScopedLock sl (midiCallbackLock);
            midiCallbacks.remove (i);
        }
    }
}

void MidiEngine::removeMidiInputCallback (MidiInputCallback* callbackToRemove)
{
    for (int i = midiCallbacks.size(); --i >= 0;)
    {
        auto& mc = midiCallbacks.getReference (i);

        if (mc.callback == callbackToRemove)
        {
            const ScopedLock sl (midiCallbackLock);
            midiCallbacks.remove (i);
        }
    }
}

void MidiEngine::handleIncomingMidiMessageInt (MidiInput* source, const MidiMessage& message)
{
    if (! message.isActiveSense())
    {
        const ScopedLock sl (midiCallbackLock);
        for (auto& mc : midiCallbacks)
            if (mc.consumer || mc.device.isEmpty() || mc.device == source->getIdentifier())
                mc.callback->handleIncomingMidiMessage (source, message);
    }
}

void MidiEngine::processMidiBuffer (const MidiBuffer& buffer, int nframes, double sampleRate)
{
    MidiMessage message;
    const double timeNow = 1.5 + Time::getMillisecondCounterHiRes();

    const ScopedLock sl (midiCallbackLock);
    for (auto m : buffer)
    {
        if (m.samplePosition >= nframes)
            break;
        message = m.getMessage();
        message.setTimeStamp (timeNow + (1000.0 * (static_cast<double> (m.samplePosition) / sampleRate)));
        for (auto& mc : midiCallbacks)
            mc.callback->handleIncomingMidiMessage (nullptr, message);
    }
}

int MidiEngine::getNumActiveMidiInputs() const
{
    int total = 0;
    for (const auto& dev : MidiInput::getAvailableDevices())
        if (isMidiInputEnabled (dev))
            ++total;
    return total;
}

//==============================================================================
void MidiEngine::setDefaultMidiOutput (const MidiDeviceInfo& device)
{
    if (defaultMidiOutputID != device.identifier)
    {
        std::unique_ptr<MidiOutput> newMidiOut;

        if (device.identifier.isNotEmpty())
            newMidiOut = MidiOutput::openDevice (device.identifier);

        if (newMidiOut)
        {
            newMidiOut->startBackgroundThread();
            {
                ScopedLock sl (midiOutputLock);
                defaultMidiOutput.swap (newMidiOut);
            }

            if (newMidiOut) // is now the old output
            {
                newMidiOut->stopBackgroundThread();
                newMidiOut.reset();
            }
        }

        defaultMidiOutputName = device.name;
        defaultMidiOutputID = device.identifier;

        sendChangeMessage();
    }
}

} // namespace element

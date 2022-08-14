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

#include "engine/midiengine.hpp"
#include "settings.hpp"

namespace Element {

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
            if (child.hasType ("input"))
            {
                if (auto* const holder = getMidiInput (child[Tags::name], true))
                {
                    holder->active = false; // open but not active initially
                    if ((bool) child[Tags::active])
                        midiInsFromXml.add (child[Tags::name]);
                }
            }
        }

        for (auto& m : MidiInput::getDevices())
            setMidiInputEnabled (m, midiInsFromXml.contains (m));

        setDefaultMidiOutput (data["defaultMidiOutput"].toString());
    }
}

void MidiEngine::writeSettings (Settings& settings)
{
    ValueTree data ("MidiSettings");
    for (auto* const holder : openMidiInputs)
    {
        ValueTree input ("input");
        input.setProperty (Tags::name, holder->input->getName(), nullptr)
            .setProperty (Tags::active, holder->active, nullptr);
        data.appendChild (input, nullptr);
    }

    if (midiInsFromXml.size() > 0)
    {
        // Add any midi devices that have been enabled before, but which aren't currently
        // open because the device has been disconnected.
        const StringArray availableMidiDevices (MidiInput::getDevices());

        for (int i = 0; i < midiInsFromXml.size(); ++i)
        {
            if (availableMidiDevices.contains (midiInsFromXml[i], true))
                continue;
            ValueTree input ("input");
            input.setProperty (Tags::name, midiInsFromXml[i], nullptr)
                .setProperty (Tags::active, true, nullptr);
            data.appendChild (input, nullptr);
        }
    }

    data.setProperty ("defaultMidiOutput", defaultMidiOutputName, nullptr);

    if (auto xml = std::unique_ptr<XmlElement> (data.createXml()))
        settings.getUserSettings()->setValue (Settings::midiEngineKey, xml.get());
}

//==============================================================================
class MidiEngine::CallbackHandler : public AudioIODeviceCallback,
                                    public MidiInputCallback,
                                    public AudioIODeviceType::Listener
{
public:
    CallbackHandler (MidiEngine& me) noexcept : owner (me) {}

private:
    void audioDeviceIOCallback (const float** ins, int numIns, float** outs, int numOuts, int numSamples) override
    {
        ignoreUnused (ins, numIns, outs, numOuts, numSamples);
    }

    void audioDeviceAboutToStart (AudioIODevice* device) override
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
        if ((active || mc.consumer) && (mc.deviceName.isEmpty() || mc.deviceName == input->getName()))
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
MidiEngine::MidiInputHolder* MidiEngine::getMidiInput (const String& deviceName, bool openIfNotAlready)
{
    for (auto* const holder : openMidiInputs)
        if (holder->input && holder->input->getName() == deviceName)
            return holder;

    if (! openIfNotAlready)
        return nullptr;

    auto index = MidiInput::getDevices().indexOf (deviceName);
    if (index >= 0)
    {
        std::unique_ptr<MidiInputHolder> holder;
        holder.reset (new MidiInputHolder (*this));
        if (auto midiIn = MidiInput::openDevice (index, holder.get()))
        {
            holder->input.reset (midiIn.release());
            holder->input->start();
            return openMidiInputs.add (holder.release());
        }
    }

    return nullptr;
}

//==============================================================================
void MidiEngine::setMidiInputEnabled (const String& name, const bool enabled)
{
    if (enabled != isMidiInputEnabled (name))
    {
        if (enabled)
        {
            if (auto* holder = getMidiInput (name, true))
                holder->active = true;
        }
        else
        {
            if (auto* holder = getMidiInput (name, false))
                holder->active = false;
        }

        sendChangeMessage();
    }
}

bool MidiEngine::isMidiInputEnabled (const String& name) const
{
    for (auto* mi : openMidiInputs)
        if (mi->input != nullptr && mi->input->getName() == name && mi->active)
            return true;

    return false;
}

void MidiEngine::addMidiInputCallback (const String& name, MidiInputCallback* callbackToAdd, bool consumer)
{
    removeMidiInputCallback (name, callbackToAdd);

    if (name.isEmpty() || isMidiInputEnabled (name) || consumer)
    {
        if (consumer)
        {
            if (auto* holder = getMidiInput (name, true))
                ignoreUnused (holder);
        }

        MidiCallbackInfo mc;
        mc.deviceName = name;
        mc.callback = callbackToAdd;
        mc.consumer = consumer;

        const ScopedLock sl (midiCallbackLock);
        midiCallbacks.add (mc);
    }
}

void MidiEngine::removeMidiInputCallback (const String& name, MidiInputCallback* callbackToRemove)
{
    for (int i = midiCallbacks.size(); --i >= 0;)
    {
        auto& mc = midiCallbacks.getReference (i);

        if (mc.callback == callbackToRemove && mc.deviceName == name)
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
            if (mc.consumer || mc.deviceName.isEmpty() || mc.deviceName == source->getName())
                mc.callback->handleIncomingMidiMessage (source, message);
    }
}

void MidiEngine::processMidiBuffer (const MidiBuffer& buffer, int nframes, double sampleRate)
{
    MidiBuffer::Iterator iter (buffer);
    MidiMessage message;
    int frame = 0;
    const double timeNow = 1.5 + Time::getMillisecondCounterHiRes();

    const ScopedLock sl (midiCallbackLock);

    while (iter.getNextEvent (message, frame))
    {
        if (frame >= nframes)
            break;

        message.setTimeStamp (timeNow + (1000.0 * (static_cast<double> (frame) / sampleRate)));
        for (auto& mc : midiCallbacks)
            mc.callback->handleIncomingMidiMessage (nullptr, message);
    }
}

int MidiEngine::getNumActiveMidiInputs() const
{
    int total = 0;
    for (const auto& dev : MidiInput::getDevices())
        if (isMidiInputEnabled (dev))
            ++total;
    return total;
}

//==============================================================================
void MidiEngine::setDefaultMidiOutput (const String& deviceName)
{
    if (defaultMidiOutputName != deviceName)
    {
        std::unique_ptr<MidiOutput> newMidiOut;

        if (deviceName.isNotEmpty())
            newMidiOut = MidiOutput::openDevice (MidiOutput::getDevices().indexOf (deviceName));

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

        defaultMidiOutputName = deviceName;

        sendChangeMessage();
    }
}

} // namespace Element

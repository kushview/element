
#include "engine/MidiEngine.h"
#include "Settings.h"

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
                midiInsFromXml.add (child [Tags::name]);
            }
        }

        for (auto& m : MidiInput::getDevices())
            setMidiInputEnabled (m, midiInsFromXml.contains (m));

        setDefaultMidiOutput (data ["defaultMidiOutput"].toString());
    }
}

void MidiEngine::writeSettings (Settings& settings)
{
    ValueTree data ("MidiSettings");
    for (int i = 0; i < enabledMidiInputs.size(); ++i)
    {
        ValueTree input ("input");
        input.setProperty (Tags::name, enabledMidiInputs[i]->getName(), nullptr);
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
            input.setProperty (Tags::name, midiInsFromXml [i], nullptr);
            data.appendChild (input, nullptr);
        }
    }

    data.setProperty ("defaultMidiOutput", defaultMidiOutputName, nullptr);

    if (auto xml = std::unique_ptr<XmlElement> (data.createXml()))
        settings.getUserSettings()->setValue (Settings::midiEngineKey, xml.get());
}

//==============================================================================
class MidiEngine::CallbackHandler  : public AudioIODeviceCallback,
                                     public MidiInputCallback,
                                     public AudioIODeviceType::Listener
{
public:
    CallbackHandler (MidiEngine& me) noexcept  : owner (me) {}

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
void MidiEngine::setMidiInputEnabled (const String& name, const bool enabled)
{
    if (enabled != isMidiInputEnabled (name))
    {
        if (enabled)
        {
            auto index = MidiInput::getDevices().indexOf (name);

            if (index >= 0)
            {
                if (auto* midiIn = MidiInput::openDevice (index, callbackHandler.get()))
                {
                    enabledMidiInputs.add (midiIn);
                    midiIn->start();
                }
            }
        }
        else
        {
            for (int i = enabledMidiInputs.size(); --i >= 0;)
                if (enabledMidiInputs[i]->getName() == name)
                    enabledMidiInputs.remove (i);
        }

        sendChangeMessage();
    }
}

bool MidiEngine::isMidiInputEnabled (const String& name) const
{
    for (auto* mi : enabledMidiInputs)
        if (mi->getName() == name)
            return true;

    return false;
}

void MidiEngine::addMidiInputCallback (const String& name, MidiInputCallback* callbackToAdd)
{
    removeMidiInputCallback (name, callbackToAdd);

    if (name.isEmpty() || isMidiInputEnabled (name))
    {
        const ScopedLock sl (midiCallbackLock);

        MidiCallbackInfo mc;
        mc.deviceName = name;
        mc.callback = callbackToAdd;
        midiCallbacks.add (mc);
    }
}

void MidiEngine::removeMidiInputCallback (const String& name, MidiInputCallback* callbackToRemove)
{
    for (int i = midiCallbacks.size(); --i >= 0;)
    {
        auto& mc = midiCallbacks.getReference(i);

        if (mc.callback == callbackToRemove && mc.deviceName == name)
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
            if (mc.deviceName.isEmpty() || mc.deviceName == source->getName())
                mc.callback->handleIncomingMidiMessage (source, message);
    }
}

//==============================================================================
void MidiEngine::setDefaultMidiOutput (const String& deviceName)
{
    if (defaultMidiOutputName != deviceName)
    {
        std::unique_ptr<MidiOutput> newMidiOut;

        if (deviceName.isNotEmpty())
            newMidiOut.reset (MidiOutput::openDevice (MidiOutput::getDevices().indexOf (deviceName)));

        if (newMidiOut)
        {
            newMidiOut->startBackgroundThread();
            {
                ScopedLock sl (getMidiOutputLock());
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

}

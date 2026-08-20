// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/devices.hpp>
#include "engine/jack.hpp"

namespace element {

using namespace juce;

const int DeviceManager::maxAudioChannels = 128;

namespace {

/** Wraps a native AudioIODeviceType so Element controls disconnect policy.

    While a device of this type is open, its name stays in the reported device
    lists even after the hardware disappears. AudioDeviceManager's internal
    availability check then still passes, so it never closes the device and
    auto-substitutes another one. Element's audio device monitor owns the
    close / wait / restore policy instead.
*/
class WatchedDeviceType : public AudioIODeviceType,
                          private AudioIODeviceType::Listener
{
public:
    WatchedDeviceType (std::unique_ptr<AudioIODeviceType> innerType, DeviceManager& dm)
        : AudioIODeviceType (innerType->getTypeName()),
          owner (dm),
          inner (std::move (innerType))
    {
        inner->addListener (this);
    }

    ~WatchedDeviceType() override { inner->removeListener (this); }

    void scanForDevices() override { inner->scanForDevices(); }

    StringArray getDeviceNames (bool wantInputNames) const override
    {
        auto names = inner->getDeviceNames (wantInputNames);

        if (auto* const device = owner.getCurrentAudioDevice())
        {
            if (device->getTypeName() == getTypeName())
            {
                const auto setup = owner.getAudioDeviceSetup();
                const auto& held = wantInputNames ? setup.inputDeviceName
                                                  : setup.outputDeviceName;
                if (held.isNotEmpty() && ! names.contains (held))
                    names.add (held);
            }
        }

        return names;
    }

    int getDefaultDeviceIndex (bool forInput) const override
    {
        return inner->getDefaultDeviceIndex (forInput);
    }

    int getIndexOfDevice (AudioIODevice* device, bool asInput) const override
    {
        return inner->getIndexOfDevice (device, asInput);
    }

    bool hasSeparateInputsAndOutputs() const override
    {
        return inner->hasSeparateInputsAndOutputs();
    }

    AudioIODevice* createDevice (const String& outputDeviceName,
                                 const String& inputDeviceName) override
    {
        return inner->createDevice (outputDeviceName, inputDeviceName);
    }

    /** Checks the unwrapped hardware list for a device name. */
    bool innerHasDevice (const String& name, bool rescan)
    {
        if (rescan)
            inner->scanForDevices();
        return inner->getDeviceNames (true).contains (name)
               || inner->getDeviceNames (false).contains (name);
    }

private:
    void audioDeviceListChanged() override
    {
        callDeviceChangeListeners();
        owner.sigDeviceListChanged();
    }

    DeviceManager& owner;
    std::unique_ptr<AudioIODeviceType> inner;
};

} // namespace

class DeviceManager::Private
{
public:
    Private (DeviceManager& o) : owner (o) {}
    ~Private() {}

    DeviceManager& owner;
    AudioEnginePtr engine;
#if ELEMENT_USE_JACK
    JackClient jack { "Element", 2, "main_in_", 2, "main_out_" };
#endif

    juce::ReferenceCountedArray<DeviceManager::LevelMeter> levelsIn, levelsOut;

    // Owned by the base class' device type list. Kept for direct access to
    // the unwrapped hardware lists (see isDevicePresent).
    juce::Array<WatchedDeviceType*> watched;
};

DeviceManager::DeviceManager()
{
    impl = std::make_unique<Private> (*this);
}

DeviceManager::~DeviceManager()
{
    closeAudioDevice();
    attach (nullptr);
}

void DeviceManager::attach (AudioEnginePtr engine)
{
    if (impl->engine == engine)
        return;

    auto old = impl->engine;

    if (old != nullptr)
    {
        removeAudioCallback (&old->getAudioIODeviceCallback());
    }

    if (engine)
    {
        addAudioCallback (&engine->getAudioIODeviceCallback());
    }
    else
    {
        closeAudioDevice();
    }

    impl->engine = engine;
}

[[maybe_unused]] static void addIfNotNull (OwnedArray<AudioIODeviceType>& list, AudioIODeviceType* const device)
{
    if (device != nullptr)
        list.add (device);
}

void DeviceManager::createAudioDeviceTypes (OwnedArray<AudioIODeviceType>& list)
{
    impl->watched.clearQuick();

    // Wrap each native type so disconnect policy stays under Element's
    // control. JACK is left unwrapped: its lifecycle is server based.
    auto addWatched = [&] (AudioIODeviceType* const type) {
        if (type == nullptr)
            return;
        auto watchedType = std::make_unique<WatchedDeviceType> (
            std::unique_ptr<AudioIODeviceType> (type), *this);
        impl->watched.add (watchedType.get());
        list.add (watchedType.release());
    };

#if JUCE_ALSA
    addWatched (AudioIODeviceType::createAudioIODeviceType_ALSA());
#endif
#if ELEMENT_USE_JACK
    addIfNotNull (list, Jack::createAudioIODeviceType (impl->jack));
#endif

    addWatched (AudioIODeviceType::createAudioIODeviceType_ASIO());
    addWatched (AudioIODeviceType::createAudioIODeviceType_WASAPI (WASAPIDeviceMode::exclusive));
    addWatched (AudioIODeviceType::createAudioIODeviceType_WASAPI (WASAPIDeviceMode::sharedLowLatency));
    addWatched (AudioIODeviceType::createAudioIODeviceType_DirectSound());

    addWatched (AudioIODeviceType::createAudioIODeviceType_CoreAudio());

    addWatched (AudioIODeviceType::createAudioIODeviceType_iOSAudio());

    addWatched (AudioIODeviceType::createAudioIODeviceType_OpenSLES());
    addWatched (AudioIODeviceType::createAudioIODeviceType_Android());
}

bool DeviceManager::isDevicePresent (const String& typeName, const String& deviceName, bool rescan)
{
    if (typeName.isEmpty() || deviceName.isEmpty())
        return false;

    for (auto* const type : impl->watched)
        if (type->getTypeName() == typeName)
            return type->innerHasDevice (deviceName, rescan);

    // Unwrapped types (e.g. JACK).
    for (auto* const type : getAvailableDeviceTypes())
    {
        if (type->getTypeName() != typeName)
            continue;
        if (rescan)
            type->scanForDevices();
        return type->getDeviceNames (true).contains (deviceName)
               || type->getDeviceNames (false).contains (deviceName);
    }

    return false;
}

void DeviceManager::getAudioDrivers (StringArray& drivers)
{
    const OwnedArray<AudioIODeviceType>& types (getAvailableDeviceTypes());
    for (int i = 0; i < types.size(); ++i)
        drivers.add (types.getUnchecked (i)->getTypeName());
}

void DeviceManager::selectAudioDriver (const String& name)
{
    setCurrentAudioDeviceType (name, true);
}

#if KV_JACK_AUDIO
kv::JackClient& DeviceManager::getJackClient()
{
    return impl->jack;
}
#endif

} // namespace element

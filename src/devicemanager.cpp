// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <functional>

#include <element/devices.hpp>
#include "engine/jack.hpp"

#if JUCE_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbt.h>
#endif

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

#if JUCE_WINDOWS

/** Watches WM_DEVICECHANGE for audio-class hardware arrival and removal.

    ASIO device types never report list changes on hot plug — their scan only
    reads the registry — so this is the sole signal that ASIO hardware
    actually appeared or disappeared. Registration is filtered to
    KSCATEGORY_AUDIO device interfaces so unrelated hardware (USB sticks,
    mice) does not trigger callbacks.

    The window is created on the message thread and JUCE pumps all messages
    there, so the WndProc and the debounced callback both run on the message
    thread; no cross-thread marshalling is needed.

    A hidden top-level window is required: message-only (HWND_MESSAGE)
    windows never receive WM_DEVICECHANGE broadcasts.
*/
class AudioHardwareWatcher : private Timer
{
public:
    explicit AudioHardwareWatcher (std::function<void()> callback)
        : onChange (std::move (callback))
    {
        JUCE_ASSERT_MESSAGE_THREAD

        const auto className = "ElementHWWatch_"
                               + String::toHexString (Time::getHighResolutionTicks());
        instance = (HINSTANCE) Process::getCurrentModuleInstanceHandle();

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof (wc);
        wc.lpfnWndProc = wndProc;
        wc.hInstance = instance;
        wc.lpszClassName = className.toWideCharPointer();
        atom = RegisterClassExW (&wc);
        if (atom == 0)
            return;

        hwnd = CreateWindowExW (0, className.toWideCharPointer(), L"", 0, 0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
        if (hwnd == nullptr)
            return;

        SetWindowLongPtrW (hwnd, GWLP_USERDATA, (LONG_PTR) this);

        // KSCATEGORY_AUDIO: the device interface class registered by audio
        // hardware drivers.
        static const GUID kscategoryAudio = {
            0x6994AD04, 0x93EF, 0x11D0, { 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96 }
        };

        DEV_BROADCAST_DEVICEINTERFACE_W filter = {};
        filter.dbcc_size = sizeof (filter);
        filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        filter.dbcc_classguid = kscategoryAudio;
        notify = RegisterDeviceNotificationW (hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
    }

    ~AudioHardwareWatcher() override
    {
        stopTimer();
        if (notify != nullptr)
            UnregisterDeviceNotification (notify);
        if (hwnd != nullptr)
            DestroyWindow (hwnd);
        if (atom != 0)
            UnregisterClassW ((LPCWSTR) MAKEINTATOM (atom), instance);
    }

private:
    static LRESULT CALLBACK wndProc (HWND h, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_DEVICECHANGE
            && (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE))
        {
            // Restarting the timer coalesces bursts, e.g. composite devices
            // exposing several interfaces.
            if (auto* const self = reinterpret_cast<AudioHardwareWatcher*> (
                    GetWindowLongPtrW (h, GWLP_USERDATA)))
                self->startTimer (500);
        }

        return DefWindowProcW (h, message, wParam, lParam);
    }

    void timerCallback() override
    {
        stopTimer();
        onChange();
    }

    std::function<void()> onChange;
    HINSTANCE instance {};
    ATOM atom {};
    HWND hwnd {};
    HDEVNOTIFY notify {};
};

#endif // JUCE_WINDOWS

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

#if JUCE_WINDOWS
    std::unique_ptr<AudioHardwareWatcher> hardwareWatcher;
#endif
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

#if JUCE_WINDOWS
    // ASIO cannot report hot plug itself; watch the OS instead.
    if (impl->hardwareWatcher == nullptr)
        impl->hardwareWatcher = std::make_unique<AudioHardwareWatcher> (
            [this] { sigDeviceListChanged(); });
#endif

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

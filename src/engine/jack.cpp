/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#if ELEMENT_USE_JACK

#include <jack/weakjack.h>
#include <jack/jack.h>

#include "engine/jack.hpp"
#include "dynlib.h"

using namespace juce;

namespace element {

static void* juce_libjackHandle = nullptr;

static void* juce_loadJackFunction (const char* const name)
{
    if (juce_libjackHandle == nullptr)
        return nullptr;

    return dlsym (juce_libjackHandle, name);
}

#define JUCE_DECL_JACK_FUNCTION(return_type, fn_name, argument_types, arguments) \
    return_type fn_name argument_types                                           \
    {                                                                            \
        using ReturnType = return_type;                                          \
        typedef return_type (*fn_type) argument_types;                           \
        static fn_type fn = (fn_type) juce_loadJackFunction (#fn_name);          \
        jassert (fn != nullptr);                                                 \
        return (fn != nullptr) ? ((*fn) arguments) : ReturnType();               \
    }

#define JUCE_DECL_VOID_JACK_FUNCTION(fn_name, argument_types, arguments) \
    void fn_name argument_types                                          \
    {                                                                    \
        typedef void (*fn_type) argument_types;                          \
        static fn_type fn = (fn_type) juce_loadJackFunction (#fn_name);  \
        jassert (fn != nullptr);                                         \
        if (fn != nullptr)                                               \
            (*fn) arguments;                                             \
    }

//==============================================================================
JUCE_DECL_JACK_FUNCTION (jack_client_t*, jack_client_open, (const char* client_name, jack_options_t options, jack_status_t* status, ...), (client_name, options, status))
JUCE_DECL_JACK_FUNCTION (int, jack_client_close, (jack_client_t * client), (client))
JUCE_DECL_JACK_FUNCTION (int, jack_activate, (jack_client_t * client), (client))
JUCE_DECL_JACK_FUNCTION (int, jack_deactivate, (jack_client_t * client), (client))
JUCE_DECL_JACK_FUNCTION (jack_nframes_t, jack_get_buffer_size, (jack_client_t * client), (client))
JUCE_DECL_JACK_FUNCTION (jack_nframes_t, jack_get_sample_rate, (jack_client_t * client), (client))
JUCE_DECL_VOID_JACK_FUNCTION (jack_on_shutdown, (jack_client_t * client, void (*function) (void* arg), void* arg), (client, function, arg))
JUCE_DECL_VOID_JACK_FUNCTION (jack_on_info_shutdown, (jack_client_t * client, JackInfoShutdownCallback function, void* arg), (client, function, arg))
JUCE_DECL_JACK_FUNCTION (void*, jack_port_get_buffer, (jack_port_t * port, jack_nframes_t nframes), (port, nframes))
JUCE_DECL_JACK_FUNCTION (jack_nframes_t, jack_port_get_total_latency, (jack_client_t * client, jack_port_t* port), (client, port))
JUCE_DECL_JACK_FUNCTION (jack_port_t*, jack_port_register, (jack_client_t * client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size), (client, port_name, port_type, flags, buffer_size))
JUCE_DECL_VOID_JACK_FUNCTION (jack_set_error_function, (void (*func) (const char*)), (func))
JUCE_DECL_JACK_FUNCTION (int, jack_set_process_callback, (jack_client_t * client, JackProcessCallback process_callback, void* arg), (client, process_callback, arg))
JUCE_DECL_JACK_FUNCTION (const char**, jack_get_ports, (jack_client_t * client, const char* port_name_pattern, const char* type_name_pattern, unsigned long flags), (client, port_name_pattern, type_name_pattern, flags))
JUCE_DECL_JACK_FUNCTION (int, jack_connect, (jack_client_t * client, const char* source_port, const char* destination_port), (client, source_port, destination_port))
JUCE_DECL_JACK_FUNCTION (const char*, jack_port_name, (const jack_port_t* port), (port))
JUCE_DECL_JACK_FUNCTION (void*, jack_set_port_connect_callback, (jack_client_t * client, JackPortConnectCallback connect_callback, void* arg), (client, connect_callback, arg))
JUCE_DECL_JACK_FUNCTION (jack_port_t*, jack_port_by_id, (jack_client_t * client, jack_port_id_t port_id), (client, port_id))
JUCE_DECL_JACK_FUNCTION (int, jack_port_connected, (const jack_port_t* port), (port))
JUCE_DECL_JACK_FUNCTION (int, jack_port_connected_to, (const jack_port_t* port, const char* port_name), (port, port_name))
JUCE_DECL_JACK_FUNCTION (int, jack_set_xrun_callback, (jack_client_t * client, JackXRunCallback xrun_callback, void* arg), (client, xrun_callback, arg))
JUCE_DECL_JACK_FUNCTION (int, jack_port_flags, (const jack_port_t* port), (port))
JUCE_DECL_JACK_FUNCTION (jack_port_t*, jack_port_by_name, (jack_client_t * client, const char* name), (client, name))

JUCE_DECL_JACK_FUNCTION (int, jack_client_name_size, (), ());
JUCE_DECL_JACK_FUNCTION (int, jack_port_name_size, (), ());
JUCE_DECL_JACK_FUNCTION (char*, jack_get_client_name, (jack_client_t * client), (client));

JUCE_DECL_VOID_JACK_FUNCTION (jack_free, (void* ptr), (ptr))

#if JUCE_DEBUG
#define JACK_LOGGING_ENABLED 1
#endif

#if JACK_LOGGING_ENABLED
namespace {
void jack_Log (const String& s)
{
    std::cerr << s << std::endl;
}

const char* getJackErrorMessage (const jack_status_t status)
{
    if (status & JackServerFailed
        || status & JackServerError)
        return "Unable to connect to JACK server";
    if (status & JackVersionError)
        return "Client's protocol version does not match";
    if (status & JackInvalidOption)
        return "The operation contained an invalid or unsupported option";
    if (status & JackNameNotUnique)
        return "The desired client name was not unique";
    if (status & JackNoSuchClient)
        return "Requested client does not exist";
    if (status & JackInitFailure)
        return "Unable to initialize client";
    return nullptr;
}
} // namespace
#define JUCE_JACK_LOG_STATUS(x)                      \
    {                                                \
        if (const char* m = getJackErrorMessage (x)) \
            jack_Log (m);                            \
    }
#define JUCE_JACK_LOG(x) jack_Log (x)
#else
#define JUCE_JACK_LOG_STATUS(x) \
    {                           \
    }
#define JUCE_JACK_LOG(x) \
    {                    \
    }
#endif

//==============================================================================
#ifndef EL_JACK_CLIENT_NAME
#ifdef JucePlugin_Name
#define EL_JACK_CLIENT_NAME JucePlugin_Name
#else
#define EL_JACK_CLIENT_NAME "Element"
#endif
#endif

struct JackPortIterator
{
    JackPortIterator (jack_client_t* const client, const bool forInput)
    {
        if (client != nullptr)
            ports.reset (element::jack_get_ports (
                client, nullptr, JACK_DEFAULT_AUDIO_TYPE, forInput ? JackPortIsInput : JackPortIsOutput));
    }

    bool next()
    {
        if (ports == nullptr || ports.get()[index + 1] == nullptr)
            return false;

        name = CharPointer_UTF8 (ports.get()[++index]);
        return true;
    }

    String getClientName() const
    {
        return name.upToFirstOccurrenceOf (":", false, false);
    }

    String getChannelName() const
    {
        return name.fromFirstOccurrenceOf (":", false, false);
    }

    struct Free
    {
        void operator() (const char** ptr) const noexcept { element::jack_free (ptr); }
    };

    std::unique_ptr<const char*, Free> ports;
    int index = -1;
    String name;
};

inline static void kv_jack_log (const char* msg)
{
    std::cerr << "[jack] " << msg << std::endl;
}

[[maybe_unused]] inline static void kv_jack_dump_error (const jack_status_t status)
{
    if ((status & JackServerFailed) || (status & JackServerError))
        kv_jack_log ("Unable to connect to JACK server");
    else if (status & JackVersionError)
        kv_jack_log ("Client's protocol version does not match");
    else if (status & JackInvalidOption)
        kv_jack_log ("The operation contained an invalid or unsupported option");
    else if (status & JackNameNotUnique)
        kv_jack_log ("The desired client name was not unique");
    else if (status & JackNoSuchClient)
        kv_jack_log ("Requested client does not exist");
    else if (status & JackInitFailure)
        kv_jack_log ("Unable to initialize client");
    else
        kv_jack_log ("Unknown client error");
}

#define returnValueIfNull(ptr, val) \
    if (ptr == nullptr)             \
    return val

const char* Jack::audioPort = JACK_DEFAULT_AUDIO_TYPE;
const char* Jack::midiPort = JACK_DEFAULT_MIDI_TYPE;

int Jack::getClientNameSize()
{
    return element::jack_client_name_size();
}

int Jack::getPortNameSize()
{
    return element::jack_port_name_size();
}

JackClient::JackClient (const String& _name,
                        int numMainInputs,
                        const String& mainInputPrefix,
                        int numMainOutputs,
                        const String& mainOutputPrefix)
    : client (nullptr),
      name (_name.isEmpty() ? "Element" : _name),
      mainInPrefix (mainInputPrefix),
      mainOutPrefix (mainOutputPrefix),
      numMainIns (numMainInputs),
      numMainOuts (numMainOutputs)
{
}

JackClient::~JackClient()
{
    this->close();
}

JackStatus JackClient::open (int opts)
{
    JackStatus status;
    client = element::jack_client_open (name.toUTF8(), (jack_options_t) opts, &status);
    return status;
}

String JackClient::close()
{
    returnValueIfNull (client, "JACK server not running");

    String error;

    if (int e = deactivate())
    {
        error << "JACK deactivation error on client close: " << e;
    }

    element::jack_client_close (client);
    client = nullptr;

    return error;
}

bool JackClient::isOpen() const
{
    return (client != nullptr);
}

int JackClient::activate()
{
    returnValueIfNull (client, -1);
    return element::jack_activate (client);
}

int JackClient::deactivate()
{
    returnValueIfNull (client, -1);
    return element::jack_deactivate (client);
}

bool JackClient::isActive()
{
    return (isOpen());
}

JackPort::Ptr JackClient::registerPort (const String& name, const String& type, int flags, int bufferSize)
{
    returnValueIfNull (client, nullptr);

    String portName = name;
    if (portName.length() >= Jack::getPortNameSize())
        portName = portName.substring (0, Jack::getPortNameSize());

    auto* const cport = element::jack_port_register (
        client, portName.toUTF8(), type.toUTF8(), flags, bufferSize);

    return cport != nullptr ? new JackPort (*this, cport) : nullptr;
}

String JackClient::getName()
{
    returnValueIfNull (client, String());
    auto nameStr = element::jack_get_client_name (client);
    auto ret = String::fromUTF8 (nameStr);
    element::jack_free (nameStr);
    return ret;
}

int JackClient::getSampleRate()
{
    returnValueIfNull (client, 0);
    return element::jack_get_sample_rate (client);
}

int JackClient::getBufferSize()
{
    returnValueIfNull (client, 0);
    return element::jack_get_buffer_size (client);
}

void JackClient::getPorts (StringArray& dest, String nameRegex, String typeRegex, uint64_t flags)
{
    dest.clear();

    if (const char** ports = element::jack_get_ports (client, nameRegex.toUTF8(), typeRegex.toUTF8(), flags))
    {
        for (int j = 0; ports[j] != 0; ++j)
            dest.add (ports[j]);
        jack_free (ports);
    }
}

//=============================================================================

JackPort::JackPort (JackClient& c, jack_port_t* p)
    : client (c), port (p) {}
JackPort::~JackPort() { port = nullptr; }

void* JackPort::getBuffer (uint32_t nframes) { return element::jack_port_get_buffer (port, nframes); }
const char* JackPort::getName() const { return element::jack_port_name (port); }

bool JackPort::isInput() const { return getFlags() & JackPortIsInput; }
bool JackPort::isOutput() const { return getFlags() & JackPortIsOutput; }
bool JackPort::isAudio() const { return true; }
bool JackPort::isMidi() const { return false; }

int JackPort::connect (const JackPort& other) { return jack_connect (client, getName(), other.getName()); }

int JackPort::getFlags() const { return element::jack_port_flags (port); }

//==============================================================================
class JackAudioIODevice : public AudioIODevice
{
public:
    JackAudioIODevice (JackClient& _client,
                       const String& inName,
                       const String& outName,
                       std::function<void()> notifyIn)
        : AudioIODevice (outName.isEmpty() ? inName : outName, "JACK"),
          inputName (inName),
          outputName (outName),
          client (_client),
          notifyChannelsChanged (std::move (notifyIn))
    {
        jassert (outName.isNotEmpty() || inName.isNotEmpty());
        jack_status_t status = client.open (JackNoStartServer);
        juce::ignoreUnused (status);

        if (! client.isOpen())
        {
            JUCE_JACK_LOG_STATUS (status);
        }
        else
        {
            element::jack_set_error_function (errorCallback);

            // open input ports
            const StringArray inputChannels (getInputChannelNames());
            for (int i = 0; i < inputChannels.size(); ++i)
            {
                String inputChannelName (client.getMainInputPrefix());
                if (inputChannelName.isEmpty())
                    inputChannelName << "in_";
                inputChannelName << ++totalNumberOfInputChannels;

                inputPorts.add (element::jack_port_register (
                    client, inputChannelName.toUTF8(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0));
            }

            // open output ports
            const StringArray outputChannels (getOutputChannelNames());
            for (int i = 0; i < outputChannels.size(); ++i)
            {
                String outputChannelName (client.getMainOutputPrefix());
                if (outputChannelName.isEmpty())
                    outputChannelName << "out_";
                outputChannelName << ++totalNumberOfOutputChannels;

                outputPorts.add (element::jack_port_register (
                    client, outputChannelName.toUTF8(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0));
            }

            inChans.calloc (totalNumberOfInputChannels + 2);
            outChans.calloc (totalNumberOfOutputChannels + 2);
        }
    }

    ~JackAudioIODevice() override
    {
        close();
        if (client.isOpen())
            client.close();
    }

    StringArray getChannelNames (const String& clientName, bool forInput) const
    {
        StringArray names;

        for (JackPortIterator i (client, forInput); i.next();)
            if (i.getClientName() == clientName)
                names.add (i.getChannelName());

        return names;
    }

    StringArray getOutputChannelNames() override { return getChannelNames (outputName, true); }
    StringArray getInputChannelNames() override { return getChannelNames (inputName, false); }

    Array<double> getAvailableSampleRates() override
    {
        Array<double> rates;

        if (client != nullptr)
            rates.add (element::jack_get_sample_rate (client));

        return rates;
    }

    Array<int> getAvailableBufferSizes() override
    {
        Array<int> sizes;

        if (client != nullptr)
            sizes.add (static_cast<int> (element::jack_get_buffer_size (client)));

        return sizes;
    }

    int getDefaultBufferSize() override { return getCurrentBufferSizeSamples(); }
    int getCurrentBufferSizeSamples() override { return client != nullptr ? static_cast<int> (element::jack_get_buffer_size (client)) : 0; }
    double getCurrentSampleRate() override { return client != nullptr ? static_cast<int> (element::jack_get_sample_rate (client)) : 0; }

    template <typename Fn>
    void forEachClientChannel (const String& clientName, bool isInput, Fn&& fn)
    {
        auto index = 0;

        for (JackPortIterator i (client, isInput); i.next();)
        {
            if (i.getClientName() != clientName)
                continue;

            fn (i.ports.get()[i.index], index);
            index += 1;
        }
    }

    String open (const BigInteger& inputChannels, const BigInteger& outputChannels, double /* sampleRate */, int /* bufferSizeSamples */) override
    {
        if (client == nullptr)
        {
            lastError = "No JACK client running";
            return lastError;
        }

        lastError.clear();
        close();

        xruns.store (0, std::memory_order_relaxed);
        element::jack_set_process_callback (client, processCallback, this);
        element::jack_set_port_connect_callback (client, portConnectCallback, this);
        element::jack_on_shutdown (client, shutdownCallback, this);
        element::jack_on_info_shutdown (client, infoShutdownCallback, this);
        element::jack_set_xrun_callback (client, xrunCallback, this);
        element::jack_activate (client);
        deviceIsOpen = true;

        if (! inputChannels.isZero())
        {
            forEachClientChannel (inputName, false, [&] (const char* portName, int index) {
                if (! inputChannels[index])
                    return;

                jassert (index < inputPorts.size());

                const auto* source = portName;
                const auto* inputPort = inputPorts[index];

                jassert (element::jack_port_flags (element::jack_port_by_name (client, source)) & JackPortIsOutput);
                jassert (element::jack_port_flags (inputPort) & JackPortIsInput);

                auto error = element::jack_connect (client, source, element::jack_port_name (inputPort));
                if (error != 0)
                    JUCE_JACK_LOG ("Cannot connect input port " + String (index) + " (" + portName + "), error " + String (error));
            });
        }

        if (! outputChannels.isZero())
        {
            forEachClientChannel (outputName, true, [&] (const char* portName, int index) {
                if (! outputChannels[index])
                    return;

                jassert (index < outputPorts.size());

                const auto* outputPort = outputPorts[index];
                const auto* destination = portName;

                jassert (element::jack_port_flags (outputPort) & JackPortIsOutput);
                jassert (element::jack_port_flags (element::jack_port_by_name (client, destination)) & JackPortIsInput);

                auto error = element::jack_connect (client, element::jack_port_name (outputPort), destination);
                if (error != 0)
                    JUCE_JACK_LOG ("Cannot connect output port " + String (index) + " (" + portName + "), error " + String (error));
            });
        }

        updateActivePorts();

        return lastError;
    }

    void close() override
    {
        stop();

        if (client != nullptr)
        {
            const auto result = element::jack_deactivate (client);
            jassertquiet (result == 0);

            element::jack_set_xrun_callback (client, xrunCallback, nullptr);
            element::jack_set_process_callback (client, processCallback, nullptr);
            element::jack_set_port_connect_callback (client, portConnectCallback, nullptr);
            element::jack_on_shutdown (client, shutdownCallback, nullptr);
            element::jack_on_info_shutdown (client, infoShutdownCallback, nullptr);
        }

        deviceIsOpen = false;
    }

    void start (AudioIODeviceCallback* newCallback) override
    {
        if (deviceIsOpen && newCallback != callback)
        {
            if (newCallback != nullptr)
                newCallback->audioDeviceAboutToStart (this);

            AudioIODeviceCallback* const oldCallback = callback;

            {
                const ScopedLock sl (callbackLock);
                callback = newCallback;
            }

            if (oldCallback != nullptr)
                oldCallback->audioDeviceStopped();
        }
    }

    void stop() override
    {
        start (nullptr);
    }

    bool isOpen() override { return deviceIsOpen; }
    bool isPlaying() override { return callback != nullptr; }
    int getCurrentBitDepth() override { return 32; }
    String getLastError() override { return lastError; }
    int getXRunCount() const noexcept override { return xruns.load (std::memory_order_relaxed); }

    BigInteger getActiveOutputChannels() const override { return activeOutputChannels; }
    BigInteger getActiveInputChannels() const override { return activeInputChannels; }

    int getOutputLatencyInSamples() override
    {
        int latency = 0;

        for (int i = 0; i < outputPorts.size(); i++)
            latency = jmax (latency, (int) element::jack_port_get_total_latency (client, outputPorts[i]));

        return latency;
    }

    int getInputLatencyInSamples() override
    {
        int latency = 0;

        for (int i = 0; i < inputPorts.size(); i++)
            latency = jmax (latency, (int) element::jack_port_get_total_latency (client, inputPorts[i]));

        return latency;
    }

    String inputName, outputName;

private:
    //==============================================================================
    class MainThreadDispatcher : private AsyncUpdater
    {
    public:
        explicit MainThreadDispatcher (JackAudioIODevice& device) : ref (device) {}
        ~MainThreadDispatcher() override { cancelPendingUpdate(); }

        void updateActivePorts()
        {
            if (MessageManager::getInstance()->isThisTheMessageThread())
                handleAsyncUpdate();
            else
                triggerAsyncUpdate();
        }

    private:
        void handleAsyncUpdate() override { ref.updateActivePorts(); }

        JackAudioIODevice& ref;
    };

    //==============================================================================
    void process (const int numSamples)
    {
        int numActiveInChans = 0, numActiveOutChans = 0;

        for (int i = 0; i < totalNumberOfInputChannels; ++i)
        {
            if (activeInputChannels[i])
                if (auto* in = (jack_default_audio_sample_t*) element::jack_port_get_buffer (inputPorts.getUnchecked (i),
                                                                                             static_cast<jack_nframes_t> (numSamples)))
                    inChans[numActiveInChans++] = (float*) in;
        }

        for (int i = 0; i < totalNumberOfOutputChannels; ++i)
        {
            if (activeOutputChannels[i])
                if (auto* out = (jack_default_audio_sample_t*) element::jack_port_get_buffer (outputPorts.getUnchecked (i),
                                                                                              static_cast<jack_nframes_t> (numSamples)))
                    outChans[numActiveOutChans++] = (float*) out;
        }

        const ScopedLock sl (callbackLock);

        if (callback != nullptr)
        {
            if ((numActiveInChans + numActiveOutChans) > 0)
                callback->audioDeviceIOCallbackWithContext (inChans.getData(),
                                                            numActiveInChans,
                                                            outChans,
                                                            numActiveOutChans,
                                                            numSamples,
                                                            {});
        }
        else
        {
            for (int i = 0; i < numActiveOutChans; ++i)
                juce::zeromem (outChans[i], static_cast<size_t> (numSamples) * sizeof (float));
        }
    }

    static int processCallback (jack_nframes_t nframes, void* callbackArgument)
    {
        if (callbackArgument != nullptr)
            ((JackAudioIODevice*) callbackArgument)->process (static_cast<int> (nframes));

        return 0;
    }

    static int xrunCallback (void* callbackArgument)
    {
        if (callbackArgument != nullptr)
            ((JackAudioIODevice*) callbackArgument)->xruns++;

        return 0;
    }

    void updateActivePorts()
    {
        BigInteger newOutputChannels, newInputChannels;

        for (int i = 0; i < outputPorts.size(); ++i)
            if (element::jack_port_connected (outputPorts.getUnchecked (i)))
                newOutputChannels.setBit (i);

        for (int i = 0; i < inputPorts.size(); ++i)
            if (element::jack_port_connected (inputPorts.getUnchecked (i)))
                newInputChannels.setBit (i);

        if (newOutputChannels != activeOutputChannels
            || newInputChannels != activeInputChannels)
        {
            AudioIODeviceCallback* const oldCallback = callback;

            stop();

            activeOutputChannels = newOutputChannels;
            activeInputChannels = newInputChannels;

            if (oldCallback != nullptr)
                start (oldCallback);

            if (notifyChannelsChanged != nullptr)
                notifyChannelsChanged();
        }
    }

    static void portConnectCallback (jack_port_id_t, jack_port_id_t, int, void* arg)
    {
        if (JackAudioIODevice* device = static_cast<JackAudioIODevice*> (arg))
            device->mainThreadDispatcher.updateActivePorts();
    }

    static void threadInitCallback (void* /* callbackArgument */)
    {
        JUCE_JACK_LOG ("JackAudioIODevice::initialise");
    }

    static void shutdownCallback (void* callbackArgument)
    {
        JUCE_JACK_LOG ("JackAudioIODevice::shutdown");

        if (JackAudioIODevice* device = (JackAudioIODevice*) callbackArgument)
        {
            device->close();
        }
    }

    static void infoShutdownCallback (jack_status_t code, [[maybe_unused]] const char* reason, void* arg)
    {
        jassertquiet (code == 0);

        JUCE_JACK_LOG ("Shutting down with message:");
        JUCE_JACK_LOG (reason);

        shutdownCallback (arg);
    }

    static void errorCallback ([[maybe_unused]] const char* msg)
    {
        JUCE_JACK_LOG ("JackAudioIODevice::errorCallback " + String (msg));
    }

    JackClient& client;
    bool deviceIsOpen = false;
    String lastError;
    AudioIODeviceCallback* callback = nullptr;
    CriticalSection callbackLock;

    HeapBlock<float*> inChans, outChans;
    int totalNumberOfInputChannels = 0;
    int totalNumberOfOutputChannels = 0;
    Array<jack_port_t*> inputPorts, outputPorts;
    BigInteger activeInputChannels, activeOutputChannels;

    std::atomic<int> xruns { 0 };

    std::function<void()> notifyChannelsChanged;
    MainThreadDispatcher mainThreadDispatcher { *this };
};

//==============================================================================
class JackAudioIODeviceType;

class JackAudioIODeviceType : public AudioIODeviceType
{
public:
    JackAudioIODeviceType (JackClient& client)
        : AudioIODeviceType ("JACK"),
          _client (client)
    {
    }

    void scanForDevices()
    {
        hasScanned = true;
        inputNames.clear();
        outputNames.clear();

#if JUCE_LINUX || JUCE_BSD
        if (juce_libjackHandle == nullptr)
            juce_libjackHandle = dlopen ("libjack.so.0", RTLD_LAZY);
        if (juce_libjackHandle == nullptr)
            juce_libjackHandle = dlopen ("libjack.so", RTLD_LAZY);
#elif JUCE_WINDOWS
        if (juce_libjackHandle == nullptr)
            juce_libjackHandle = dlopen ("libjack64.dll", RTLD_NOW | RTLD_LOCAL);
#elif JUCE_MAC
        if (juce_libjackHandle == nullptr)
            juce_libjackHandle = dlopen ("/usr/local/lib/libjack.0.dylib", RTLD_LAZY | RTLD_LOCAL);
#endif

        if (juce_libjackHandle == nullptr)
        {
            std::clog << "[jack] could not load library.\n";
            return;
        }

        jack_status_t status = {};

        // open a dummy client
        if (auto* const client = element::jack_client_open ("JuceJackDummy", JackNoStartServer, &status))
        {
            // scan for output devices
            for (JackPortIterator i (client, false); i.next();)
                if (i.getClientName() != (EL_JACK_CLIENT_NAME) && ! inputNames.contains (i.getClientName()))
                    inputNames.add (i.getClientName());

            // scan for input devices
            for (JackPortIterator i (client, true); i.next();)
                if (i.getClientName() != (EL_JACK_CLIENT_NAME) && ! outputNames.contains (i.getClientName()))
                    outputNames.add (i.getClientName());

            element::jack_client_close (client);
        }
        else
        {
            JUCE_JACK_LOG_STATUS (status);
        }
    }

    StringArray getDeviceNames (bool wantInputNames) const
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this
        return wantInputNames ? inputNames : outputNames;
    }

    int getDefaultDeviceIndex (bool /* forInput */) const
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this
        return 0;
    }

    bool hasSeparateInputsAndOutputs() const { return true; }

    int getIndexOfDevice (AudioIODevice* device, bool asInput) const
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        if (JackAudioIODevice* d = dynamic_cast<JackAudioIODevice*> (device))
            return asInput ? inputNames.indexOf (d->inputName)
                           : outputNames.indexOf (d->outputName);

        return -1;
    }

    AudioIODevice* createDevice (const String& outputDeviceName,
                                 const String& inputDeviceName)
    {
        jassert (hasScanned); // need to call scanForDevices() before doing this

        const int inputIndex = inputNames.indexOf (inputDeviceName);
        const int outputIndex = outputNames.indexOf (outputDeviceName);

        if (inputIndex >= 0 || outputIndex >= 0)
            return new JackAudioIODevice (_client, inputDeviceName, outputDeviceName, [this] { callDeviceChangeListeners(); });

        return nullptr;
    }

private:
    JackClient& _client;
    StringArray inputNames, outputNames;
    bool hasScanned = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JackAudioIODeviceType)
};

AudioIODeviceType* Jack::createAudioIODeviceType (JackClient& client)
{
    return new JackAudioIODeviceType (client);
}

} // namespace element

#endif

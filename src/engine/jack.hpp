// SPDX-FileCopyrightText: 2014-2019 Kushview, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if ELEMENT_USE_JACK

#include <jack/jack.h>

#include <element/juce/audio_devices.hpp>

namespace element {

class JackCallback;
class JackClient;
class JackPort;

struct Jack
{
    static const char* audioPort;
    static const char* midiPort;
    static juce::AudioIODeviceType* createAudioIODeviceType (JackClient&);
    static int getClientNameSize();
    static int getPortNameSize();
};

class JackPort final : public juce::ReferenceCountedObject
{
public:
    using Ptr = juce::ReferenceCountedObjectPtr<JackPort>;

    ~JackPort();

    void* getBuffer (uint32_t nframes);
    const char* getName() const;
    bool isInput() const;
    bool isOutput() const;
    bool isAudio() const;
    bool isMidi() const;
    int connect (const JackPort& other);
    int getFlags() const;

    operator jack_port_t*() const { return port; }

private:
    friend class JackClient;
    JackPort (JackClient& c, jack_port_t* p);
    JackClient& client;
    jack_port_t* port;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JackPort)
};

using JackStatus = jack_status_t;

class JackClient final
{
public:
    /** Make a new JACK client */
    explicit JackClient (const juce::String& name = juce::String(),
                         int numMainInputs = 2,
                         const juce::String& mainInputPrefix = "main_in_",
                         int numMainOutputs = 2,
                         const juce::String& mainOutputPrefix = "main_out_");

    ~JackClient();

    int getNumMainInputs() const { return numMainIns; }
    int getNumMainOutputs() const { return numMainOuts; }
    const juce::String& getMainOutputPrefix() const { return mainOutPrefix; }
    const juce::String& getMainInputPrefix() const { return mainInPrefix; }

    /** Open the client */
    JackStatus open (int options);

    /** Close the client */
    juce::String close();

    /** Returns true if client is open */
    bool isOpen() const;

    /** Activate the client */
    int activate();

    /** Deactivate the client */
    int deactivate();

    /** Returns true if the client is active */
    bool isActive();

    /** Register a new port */
    JackPort::Ptr registerPort (const juce::String& name, const juce::String& type, int flags, int bufsize = 0);

    /** Returns the client's name */
    juce::String getName();

    /** Returns the current sample rate */
    int getSampleRate();

    /** Returns the current buffer size */
    int getBufferSize();

    /** Query for ports */
    void getPorts (juce::StringArray& dest, juce::String nameRegex = {}, juce::String typeRegex = {}, uint64_t flags = 0);

    operator jack_client_t*() const { return client; }

private:
    JackClient (const JackClient&);
    JackClient& operator= (const JackClient&);
    jack_client_t* client;
    juce::String name, mainInPrefix, mainOutPrefix;
    int numMainIns, numMainOuts;
    juce::Array<JackPort::Ptr> ports;
};

} // namespace element
#endif

/*
    This file is part of the Kushview Modules for JUCE
    Copyright (c) 2014-2019  Kushview, LLC.  All rights reserved.

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

#pragma once

#include <cstdint>

#include <element/juce/core.hpp>
#include <element/juce/data_structures.hpp>

#ifndef EL_INVALID_CHANNEL
    #define EL_INVALID_CHANNEL -1
#endif

#ifndef EL_INVALID_PORT
    #define EL_INVALID_PORT (uint32_t) - 1
#endif

#ifndef EL_INVALID_NODE
    #define EL_INVALID_NODE EL_INVALID_PORT
#endif

namespace element {

/** The type of a port. */
class PortType {
public:
    enum ID {
        Control = 0,
        Audio = 1,
        CV = 2,
        Atom = 3,
        Event = 4,
        Midi = 5,
        Video = 6,
        Unknown = 7
    };

    PortType (const juce::Identifier& identifier)
        : type (typeForString (identifier.toString())) {}

    PortType (const juce::String& identifier)
        : type (typeForString (identifier)) {}

    PortType (ID id) : type (id) {}

    PortType (const int t) : type (static_cast<ID> (t)) {}

    PortType (const PortType& o)
    {
        type = o.type;
    }

    /** Get a URI string for this port type */
    inline const juce::String& getURI() const { return typeURI (type); }

    /** Get a human readable name for this port type */
    inline const juce::String& getName() const { return typeName (type); }

    /** Get a slug version of the port type */
    inline const juce::String& getSlug() const { return slugName (type); }
    /** Get a slug version of the port type */
    inline static const juce::String& getSlug (const int t) { return slugName (static_cast<unsigned> (t)); }

    /** Get the port type id. This is useful in switch statements */
    inline ID id() const { return type; }

    inline PortType& operator= (const int& t)
    {
        jassert (t >= PortType::Control && t <= PortType::Unknown);
        type = static_cast<ID> (t);
        return *this;
    }

    inline PortType& operator= (const PortType& o)
    {
        type = o.type;
        return *this;
    }

    inline bool operator== (const ID& id) const { return (type == id); }
    inline bool operator!= (const ID& id) const { return (type != id); }
    inline bool operator== (const PortType& t) const { return (type == t.type); }
    inline bool operator!= (const PortType& t) const { return (type != t.type); }
    inline bool operator<(const PortType& t) const { return (type < t.type); }

    inline operator int() const { return (int) this->type; }

    inline bool isAudio() const { return type == Audio; }
    inline bool isControl() const { return type == Control; }
    inline bool isCv() const { return type == CV; }
    inline bool isAtom() const { return type == Atom; }
    inline bool isMidi() const { return type == Midi; }
    inline bool isEvent() const { return type == Event; }
    inline bool isVideo() const { return type == Video; }

    /** Return true if two port types can connect to one another */
    static inline bool canConnect (const PortType& sourceType, const PortType& destType)
    {
        if (sourceType == PortType::Unknown || destType == PortType::Unknown)
            return false;

        if (sourceType == destType)
            return true;

        if (sourceType == PortType::Audio && destType == PortType::CV)
            return true;

        if (sourceType == PortType::Control && destType == PortType::CV)
            return true;

        return false;
    }

    /** Return true if this port type can connect to another
        @param other The other port type
        @param isOutput Set true if 'this' is the output (source) type */
    inline bool canConnect (const PortType& other, bool isOutput = true) const
    {
        const bool res = isOutput ? canConnect (*this, other) : canConnect (other, *this);
        return res;
    }

private:
    /** @internal */
    static inline const juce::String& typeURI (unsigned id)
    {
        jassert (id <= Video);

        static const juce::String uris[] = {
            juce::String ("http://lv2plug.in/ns/lv2core#ControlPort"),
            juce::String ("http://lv2plug.in/ns/lv2core#AudioPort"),
            juce::String ("http://lv2plug.in/ns/lv2core#CVPort"),
            juce::String ("http://lv2plug.in/ns/lv2core#AtomPort"),
            juce::String ("http://lv2plug.in/ns/lv2core#EventPort"),
            juce::String ("https://kushview.net/ns/element#MidiPort"),
            juce::String ("https://kushview.net/ns/element#VideoPort"),
            juce::String ("http://lvtoolkit.org/ns/lvtk#null")
        };

        return uris[id];
    }

    /** @internal */
    static inline const juce::String& typeName (unsigned id)
    {
        jassert (id <= Video);
        static const juce::String uris[] = {
            juce::String ("Control"),
            juce::String ("Audio"),
            juce::String ("CV"),
            juce::String ("Atom"),
            juce::String ("Event"),
            juce::String ("MIDI"),
            juce::String ("Video"),
            juce::String ("Unknown")
        };
        return uris[id];
    }

    /** @internal */
    static inline const juce::String& slugName (unsigned id)
    {
        jassert (id <= Video);
        static const juce::String slugs[] = {
            juce::String ("control"),
            juce::String ("audio"),
            juce::String ("cv"),
            juce::String ("atom"),
            juce::String ("event"),
            juce::String ("midi"),
            juce::String ("video"),
            juce::String ("unknown")
        };
        return slugs[id];
    }

    static inline ID typeForString (const juce::String& identifier)
    {
        for (int i = 0; i <= Midi; ++i) {
            if (slugName (i) == identifier || typeURI (i) == identifier || typeName (i) == identifier) {
                return static_cast<ID> (i);
            }
        }
        return Unknown;
    }

    ID type;
};

/** Maps channel numbers to a port indexes for all port types. This is an attempt
    to handle boiler-plate port to channel mapping functions */
class ChannelMapping {
public:
    inline ChannelMapping() { init(); }

    /** Maps an array of port types sorted by port index, to channels */
    inline ChannelMapping (const juce::Array<PortType>& types)
    {
        init();

        for (int port = 0; port < types.size(); ++port)
            addPort (types.getUnchecked (port), (uint32_t) port);
    }

    inline void clear()
    {
        for (int i = 0; i < ports.size(); ++i)
            ports.getUnchecked (i)->clearQuick();
    }

    /** Add (append) a port to the map */
    inline void addPort (PortType type, uint32_t index)
    {
        ports.getUnchecked (type)->add (index);
    }

    inline bool containsChannel (const PortType type, const int channel) const
    {
        if (type == PortType::Unknown)
            return false;

        const juce::Array<uint32_t>* const a (ports.getUnchecked (type));
        return a->size() > 0 && juce::isPositiveAndBelow (channel, a->size());
    }

    int getNumChannels (const PortType type) const { return ports.getUnchecked (type)->size(); }
    uint32_t getNumPorts (const PortType type) const { return ports.getUnchecked (type)->size(); }

    /** Get a port index for a channel */
    inline uint32_t getPortChecked (const PortType type, const int channel) const
    {
        if (! containsChannel (type, channel))
            return EL_INVALID_PORT;

        const juce::Array<uint32_t>* const a (ports.getUnchecked (type));
        return a->getUnchecked (channel);
    }

    const juce::Array<uint32_t>& getPorts (const PortType type) const { return *ports.getUnchecked (type); }

    inline uint32_t getPort (const PortType type, const int channel) const
    {
        return ports.getUnchecked (type)->getUnchecked (channel);
    }

    inline uint32_t getAtomPort (const int channel) const { return ports.getUnchecked (PortType::Atom)->getUnchecked (channel); }
    inline uint32_t getAudioPort (const int channel) const { return ports.getUnchecked (PortType::Audio)->getUnchecked (channel); }
    inline uint32_t getControlPort (const int channel) const { return ports.getUnchecked (PortType::Control)->getUnchecked (channel); }
    inline uint32_t getCVPort (const int channel) const { return ports.getUnchecked (PortType::CV)->getUnchecked (channel); }
    inline uint32_t getEventPort (const int channel) const { return ports.getUnchecked (PortType::Event)->getUnchecked (channel); }
    inline uint32_t getMidiPort (const int channel) const { return ports.getUnchecked (PortType::Midi)->getUnchecked (channel); }

private:
    // owned arrays of arrays....
    juce::OwnedArray<juce::Array<uint32_t>> ports;

    inline void init()
    {
        ports.ensureStorageAllocated (PortType::Unknown + 1);
        for (int p = 0; p <= PortType::Unknown; ++p)
            ports.add (new juce::Array<uint32_t>());
    }
};

/** Contains two ChannelMappings.  One for inputs and one for outputs */
class ChannelConfig {
public:
    ChannelConfig() {}
    ~ChannelConfig() {}

    inline void addPort (const PortType type, const uint32_t port, const bool isInput)
    {
        ChannelMapping& mapping = isInput ? inputs : outputs;
        mapping.addPort (type, port);
    }

    inline void addInput (const PortType type, const uint32_t port) { inputs.addPort (type, port); }
    inline void addOutput (const PortType type, const uint32_t port) { outputs.addPort (type, port); }

    inline const ChannelMapping& getChannelMapping (const bool isInput) const { return isInput ? inputs : outputs; }
    inline const ChannelMapping& getInputs() const { return inputs; }
    inline const ChannelMapping& getOutputs() const { return outputs; }

    inline uint32_t getPort (PortType type, int channel, bool isInput) const { return getChannelMapping (isInput).getPort (type, channel); }
    inline uint32_t getInputPort (const PortType type, const int channel) const { return inputs.getPort (type, channel); }
    inline uint32_t getOutputPort (const PortType type, const int channel) const { return outputs.getPort (type, channel); }

    inline uint32_t getAtomPort (int channel, bool isInput) const { return getChannelMapping (isInput).getAudioPort (channel); }
    inline uint32_t getAudioPort (int channel, bool isInput) const { return getChannelMapping (isInput).getAudioPort (channel); }
    inline uint32_t getControlPort (int channel, bool isInput) const { return getChannelMapping (isInput).getAudioPort (channel); }
    inline uint32_t getCVPort (int channel, bool isInput) const { return getChannelMapping (isInput).getAudioPort (channel); }

    inline uint32_t getAudioInputPort (const int channel) const { return inputs.getAudioPort (channel); }
    inline uint32_t getAudioOutputPort (const int channel) const { return outputs.getAudioPort (channel); }
    inline uint32_t getControlInputPort (const int channel) const { return inputs.getControlPort (channel); }
    inline uint32_t getControlOutputPort (const int channel) const { return outputs.getControlPort (channel); }

    inline int getNumChannels (const PortType type, bool isInput) const
    {
        return isInput ? inputs.getNumChannels (type) : outputs.getNumChannels (type);
    }

    inline int getNumAtomInputs() const { return inputs.getNumChannels (PortType::Atom); }
    inline int getNumAtomOutputs() const { return outputs.getNumChannels (PortType::Atom); }
    inline int getNumAudioInputs() const { return inputs.getNumChannels (PortType::Audio); }
    inline int getNumAudioOutputs() const { return outputs.getNumChannels (PortType::Audio); }
    inline int getNumControlInputs() const { return inputs.getNumChannels (PortType::Control); }
    inline int getNumControlOutputs() const { return outputs.getNumChannels (PortType::Control); }
    inline int getNumCVInputs() const { return inputs.getNumChannels (PortType::CV); }
    inline int getNumCVOutputs() const { return outputs.getNumChannels (PortType::CV); }
    inline int getNumEventInputs() const { return inputs.getNumChannels (PortType::Event); }
    inline int getNumEventOutputs() const { return outputs.getNumChannels (PortType::Event); }

private:
    ChannelMapping inputs, outputs;
};

/** A detailed descption of a port */
struct PortDescription {
    PortDescription() {}
    PortDescription (int portType, int portIndex, int portChannel, const juce::String& portSymbol, const juce::String& portName, const bool isInput)
        : type (portType), index (portIndex), channel (portChannel), symbol (portSymbol), name (portName), input (isInput) {}
    PortDescription (const PortDescription& o) { operator= (o); }
    PortDescription& operator= (const PortDescription& o)
    {
        type = o.type;
        index = o.index;
        channel = o.channel;
        symbol = o.symbol;
        name = o.name;
        label = o.label;
        input = o.input;
        minValue = o.minValue;
        maxValue = o.maxValue;
        defaultValue = o.defaultValue;
        return *this;
    }

    int type { 0 };
    int index { 0 };
    int channel { 0 };
    juce::String symbol {};
    juce::String name {};
    juce::String label {};
    bool input { false };
    float minValue { 0.0 };
    float maxValue { 1.0 };
    float defaultValue { 1.0 };
};

struct PortIndexComparator {
    static int compareElements (const PortDescription* const first, const PortDescription* const second)
    {
        return (first->index < second->index) ? -1
                                              : ((second->index < first->index) ? 1
                                                                                : 0);
    }
};

class PortList {
public:
    PortList() = default;
    PortList (const PortList& o) { operator= (o); }
    PortList (PortList&& o) : ports (std::move (o.ports)) {}

    ~PortList()
    {
        ports.clear();
    }

    inline void clear() { ports.clear(); }
    inline void clearQuick() { ports.clearQuick (true); }
    inline int size() const { return ports.size(); }
    inline int size (int type, bool input) const
    {
        int n = 0;
        for (const auto* port : ports)
            if (port->type == type && port->input == input)
                ++n;
        return n;
    }

    inline void add (PortDescription* port)
    {
        jassert (port != nullptr);
        jassert (port->type >= PortType::Control && port->type < PortType::Unknown);
        jassert (nullptr == findByIndexInternal (port->index));
        jassert (nullptr == findByChannelInternal (port->type, port->channel, port->input));
        PortIndexComparator sorter;
        ports.addSorted (sorter, port);
    }

    inline void addControl (int index, int channel, const juce::String& symbol, const juce::String& name, float minValue, float maxValue, float defaultValue, bool input)
    {
        auto* const port = new PortDescription (
            PortType::Control, index, channel, symbol, name, input);
        port->minValue = minValue;
        port->maxValue = maxValue;
        port->defaultValue = defaultValue;
        add (port);
    }

    inline void add (int type, int index, int channel, const juce::String& symbol, const juce::String& name, const bool input)
    {
        add (new PortDescription (type, index, channel, symbol, name, input));
    }

    inline int getChannelForPort (const int port) const
    {
        if (auto* const desc = findByIndexInternal (port))
            return desc->channel;
        return EL_INVALID_CHANNEL;
    }

    inline int getPortForChannel (int type, int channel, bool input) const
    {
        if (auto* const desc = findByChannelInternal (type, channel, input))
            return desc->index;
        return static_cast<int> (EL_INVALID_PORT);
    }

    inline int getType (const int port) const
    {
        if (auto* const desc = findByIndexInternal (port))
            return desc->type;
        return PortType::Unknown;
    }

    inline bool isInput (const int port, const bool defaultRet = false) const
    {
        if (auto* const desc = findByIndexInternal (port))
            return desc->input;
        return defaultRet;
    }

    inline bool isOutput (const int port, const bool defaultRet = true) const
    {
        return ! isInput (port, defaultRet);
    }

    inline PortDescription getPort (int index) const
    {
        jassert (juce::isPositiveAndBelow (index, ports.size()));
        if (juce::isPositiveAndBelow (index, ports.size()))
            return *ports.getUnchecked (index);
        return {};
    }

    inline PortDescription** begin() noexcept
    {
        return ports.begin();
    }

    inline PortDescription* const* begin() const noexcept
    {
        return ports.begin();
    }

    inline PortDescription** end() noexcept
    {
        return ports.end();
    }

    inline PortDescription* const* end() const noexcept
    {
        return ports.end();
    }

    inline const juce::OwnedArray<PortDescription>& getPorts() const { return ports; }
    inline void swapWith (PortList& o) { ports.swapWith (o.ports); }

    PortList& operator= (PortList&& o)
    {
        ports = std::move (o.ports);
        return *this;
    }

    PortList& operator= (const PortList& o)
    {
        ports.clearQuick (true);
        ports.addCopiesOf (o.ports);
        return *this;
    }

private:
    juce::OwnedArray<PortDescription> ports;

    inline PortDescription* findByIndexInternal (int index) const
    {
        for (auto* port : ports)
            if (port->index == index)
                return port;
        return nullptr;
    }

    inline PortDescription* findBySymbolInternal (const juce::String& symbol) const
    {
        for (auto* port : ports)
            if (port->symbol == symbol)
                return port;
        return nullptr;
    }

    inline PortDescription* findByChannelInternal (int type, int channel, bool isInput) const
    {
        for (auto* port : ports)
            if (port->type == type && port->channel == channel && port->input == isInput)
                return port;
        return nullptr;
    }

#if JUCE_MODULE_AVAILABLE_juce_data_structures
public:
    inline juce::ValueTree createValueTree (const int port) const
    {
        if (const auto* desc = findByIndexInternal (port)) {
            juce::ValueTree data ("port");
            data.setProperty ("index", desc->index, nullptr)
                .setProperty ("channel", desc->channel, nullptr)
                .setProperty ("type", PortType::getSlug (desc->type), nullptr)
                .setProperty ("input", desc->input, nullptr)
                .setProperty ("name", desc->name, nullptr)
                .setProperty ("symbol", desc->symbol, nullptr);
            return data;
        }

        return juce::ValueTree();
    }
#endif
};

} // namespace element

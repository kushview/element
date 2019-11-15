
#include "engine/MidiPipe.h"
#include "sol/sol.hpp"

using namespace sol;

namespace Element {
namespace Lua {

static auto NS (state& lua, const char* name) { return lua[name].get_or_create<table>(); }
static auto EL (state& lua)     { return lua["el"].get_or_create<table>(); }
static auto KV (state& lua)     { return lua["kv"].get_or_create<table>(); }
static auto JUCE (state& lua)   { return lua["juce"].get_or_create<table>(); }

void registerUI (state& lua)
{
}

static MidiBuffer::Iterator midiBufferIteratorFactory (MidiBuffer& buffer)
{
    MidiBuffer::Iterator iter (buffer);
    return std::move (iter);
}

void registerEngine (state& lua)
{
    // AudioBuffer
    lua.new_usertype<AudioSampleBuffer> ("AudioBuffer", no_constructor,
        "get_num_channels", &AudioSampleBuffer::getNumChannels,
        "get_num_samples",  &AudioSampleBuffer::getNumSamples,
        "clear", overload (resolve<void()> (&AudioSampleBuffer::clear),
                           resolve<void(int,int)> (&AudioSampleBuffer::clear),
                           resolve<void(int,int,int)> (&AudioSampleBuffer::clear))
    );
  
    // MidiMessage
    lua.new_usertype<MidiMessage> ("MidiMessage", no_constructor,
        "make", []() { return MidiMessage(); },
        meta_function::to_string, [](MidiMessage& msg) {
            return msg.getDescription().toRawUTF8();
        }
    );

    // MidiBuffer
    lua.new_usertype<MidiBuffer> ("MidiBuffer", no_constructor,
        "clear",            overload (resolve<void()> (&MidiBuffer::clear),
                                      resolve<void(int, int)> (&MidiBuffer::clear)),
        "is_empty",         &MidiBuffer::isEmpty,
        "get_num_events",   &MidiBuffer::getNumEvents,
        "swap_with",        &MidiBuffer::swapWith,
        "iterator",         [](const MidiBuffer& b) {
            MidiBuffer::Iterator iter (b);
            return std::move (iter);
        }
    );

    lua.new_usertype<MidiBuffer::Iterator> ("MidiBufferIterator", no_constructor,
        "make", midiBufferIteratorFactory,
        "set_next_sample_position", &MidiBuffer::Iterator::setNextSamplePosition,
        "get_next_event", [](MidiBuffer::Iterator& iter, MidiMessage& msg) {
            int frame = 0;
            bool more = iter.getNextEvent (msg, frame);
            return std::tuple (more, frame);
        }
    );

    // MidiPipe
    lua.new_usertype<MidiPipe> ("MidiPipe", no_constructor,
        "size",             readonly_property (&MidiPipe::getNumBuffers),
        "get_num_buffers",  &MidiPipe::getNumBuffers,
        "get_read_buffer",  &MidiPipe::getReadBuffer,
        "get_write_buffer", &MidiPipe::getWriteBuffer,
        "clear",            overload (resolve<void()> (&MidiPipe::clear),
                                      resolve<void(int,int)> (&MidiPipe::clear),
                                      resolve<void(int,int,int)> (&MidiPipe::clear))
    );

    // Node
    // lua.new_usertype<Node> ("Node", no_constructor,
    // );

    // // Session
    // lua.new_usertype<Session> ("Session", no_constructor,
    // );
}

}}

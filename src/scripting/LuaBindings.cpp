
#include "engine/MidiPipe.h"
#include "sol/sol.hpp"

using namespace sol;

namespace Element {
namespace Lua {

static auto EL (state& lua)     { return lua["EL"].get_or_create<table>(); }
static auto JUCE (state& lua)   { return lua["JUCE"].get_or_create<table>(); }

void registerUI (state& lua)
{

}

void registerEngine (state& lua)
{
    auto j = JUCE (lua);
    
    // MidiMessage
    j.new_usertype<MidiMessage> ("MidiMessage", no_constructor);

    // MidiBuffer
    j.new_usertype<MidiBuffer> ("MidiBuffer", no_constructor,
        "clear",            overload (resolve<void()> (&MidiBuffer::clear),
                                      resolve<void(int, int)> (&MidiBuffer::clear)),
        "is_empty",         &MidiBuffer::isEmpty,
        "get_num_events",   &MidiBuffer::getNumEvents,
        "swap_with",        &MidiBuffer::swapWith,
        "data",             readonly_property (&MidiBuffer::data)
    );

    auto e = EL (lua);

    // MidiPipe
    e.new_usertype<MidiPipe> ("MidiPipe", no_constructor,
        "size",             readonly_property (&MidiPipe::getNumBuffers),
        "get_num_buffers",  &MidiPipe::getNumBuffers,
        "get_read_buffer",  &MidiPipe::getNumBuffers,
        "get_write_buffer", &MidiPipe::getWriteBuffer,
        "clear",            overload (resolve<void()> (&MidiPipe::clear),
                                      resolve<void(int,int)> (&MidiPipe::clear),
                                      resolve<void(int,int,int)> (&MidiPipe::clear))
    );
}

}}

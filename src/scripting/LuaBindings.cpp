
#include "sol/sol.hpp"
#include "engine/MidiPipe.h"
#include "scripting/LuaBindings.h"

namespace Element {
namespace Lua {

void registerEngine (sol::state& lua)
{
    // MidiBuffer
    auto midiBufferType = lua.new_usertype<MidiBuffer> ("MidiBuffer", sol::constructors<MidiBuffer()>());
	midiBufferType["clear"] = sol::overload (sol::resolve<void()> (&MidiBuffer::clear),
                                             sol::resolve<void(int, int)> (&MidiBuffer::clear));
    midiBufferType["empty"] = &MidiBuffer::isEmpty;
    midiBufferType["num_events"] = &MidiBuffer::getNumEvents;
    midiBufferType["swap"] = &MidiBuffer::swapWith;
    midiBufferType["data"] = sol::readonly_property(&MidiBuffer::data);

    // MidiPipe
    auto midiPipeType = lua.new_usertype<MidiPipe> ("MidiPipe", sol::constructors<MidiPipe()>());
    midiPipeType["size"] = sol::readonly_property (&MidiPipe::getNumBuffers);
    midiPipeType["num_buffers"] = &MidiPipe::getNumBuffers;
    midiPipeType["read_buffer"] = &MidiPipe::getReadBuffer;
    midiPipeType["write_buffer"] = &MidiPipe::getWriteBuffer;
    midiPipeType["clear"] = sol::overload (sol::resolve<void()> (&MidiPipe::clear),
                                           sol::resolve<void(int,int)> (&MidiPipe::clear),
                                           sol::resolve<void(int,int,int)> (&MidiPipe::clear));
}

}}

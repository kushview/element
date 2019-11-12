
#include "engine/MidiPipe.h"
#include "scripting/LuaBindings.h"
#include "sol/sol.hpp"

namespace Element {
namespace Lua {

void registerEngine (sol::state& lua)
{
    auto midiBufferType = lua.new_usertype<MidiPipe> ("MidiBuffer", sol::constructors<MidiBuffer()>());
	auto midiPipeType = lua.new_usertype<MidiPipe> ("MidiPipe", sol::constructors<MidiPipe()>());
    midiPipeType["size"] = sol::readonly_property (&MidiPipe::getNumBuffers);
    midiPipeType["read_buffer"] = &MidiPipe::getReadBuffer;
    midiPipeType["write_buffer"] = &MidiPipe::getWriteBuffer;
    midiPipeType["clear"] = sol::resolve<void()> (&MidiPipe::clear);
}

}}

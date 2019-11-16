
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
        "clear", overload (
            resolve<void()> (&AudioSampleBuffer::clear),
            resolve<void(int,int)> (&AudioSampleBuffer::clear),
            resolve<void(int,int,int)> (&AudioSampleBuffer::clear)),
        "apply_gain", overload (
            resolve<void(int,int,int,float)> (&AudioSampleBuffer::applyGain),
            resolve<void(int,int,float)> (&AudioSampleBuffer::applyGain),
            resolve<void(float)> (&AudioSampleBuffer::applyGain)),
        "apply_gain_ramp", overload (
            resolve<void(int,int,int,float,float)> (&AudioSampleBuffer::applyGainRamp),
            resolve<void(int,int,float,float)> (&AudioSampleBuffer::applyGainRamp))
    );
  
    // MidiMessage
    lua.new_usertype<MidiMessage> ("MidiMessage", no_constructor,
        "make",                     []() { return MidiMessage(); },
        meta_function::to_string,   [](MidiMessage& msg) { return msg.getDescription().toRawUTF8(); },
        "get_raw_data",             &MidiMessage::getRawData,
        "get_raw_data_size",        &MidiMessage::getRawDataSize,
        "get_description",          [](MidiMessage& msg) { return msg.getDescription().toRawUTF8(); },
        "get_time_stamp",           &MidiMessage::getTimeStamp,
        "add_to_time_stamp",        &MidiMessage::addToTimeStamp,
        "with_time_stamp",          &MidiMessage::withTimeStamp,
        "get_channel",              &MidiMessage::getChannel,
        "is_for_channel",           &MidiMessage::isForChannel,
        "set_channel",              &MidiMessage::setChannel,
        "is_sys_ex",                &MidiMessage::isSysEx,
        "get_sys_ex_data",          &MidiMessage::getSysExData,
        "get_sys_ex_data_size",     &MidiMessage::getSysExDataSize,
        "is_note_on",               overload (&MidiMessage::isNoteOn),
        "note_on",                  resolve<MidiMessage(int,int,uint8)> (MidiMessage::noteOn),
        "note_on_float",            resolve<MidiMessage(int,int,float)> (MidiMessage::noteOn),
        "is_note_off",              overload (&MidiMessage::isNoteOff),
        "note_off",                 resolve<MidiMessage(int,int,uint8)> (MidiMessage::noteOff),
        "note_off_float",           resolve<MidiMessage(int,int,float)> (MidiMessage::noteOff),
        "is_note_on_or_off",        &MidiMessage::isNoteOnOrOff,
        "get_note_number",          &MidiMessage::getNoteNumber,
        "set_note_number",          &MidiMessage::setNoteNumber,
        "get_velocity",             &MidiMessage::getVelocity,
        "get_float_velocity",       &MidiMessage::getFloatVelocity,
        "set_velocity",             &MidiMessage::setVelocity,
        "multiply_velocity",        &MidiMessage::multiplyVelocity,
        "is_sustain_pedal_on",      &MidiMessage::isSustainPedalOn,
        "is_sustain_pedal_off",     &MidiMessage::isSustainPedalOff,
        "is_sostenuto_pedal_on",    &MidiMessage::isSostenutoPedalOn,
        "is_sostenuto_pedal_off",   &MidiMessage::isSostenutoPedalOff,
        "is_soft_pedal_on",         &MidiMessage::isSoftPedalOn,
        "is_soft_pedal_off",        &MidiMessage::isSoftPedalOff,
        "is_program_change",        &MidiMessage::isProgramChange,
        "get_program_change_number", &MidiMessage::getProgramChangeNumber,
        "program_change",           &MidiMessage::programChange,
        "is_pitch_wheel",           &MidiMessage::isPitchWheel,
        "get_pitch_wheel_value",    &MidiMessage::getPitchWheelValue,
        "pitch_wheel",              &MidiMessage::pitchWheel
    );

    // MidiBuffer
    lua.new_usertype<MidiBuffer> ("MidiBuffer", no_constructor,
        "clear", overload (
            resolve<void()> (&MidiBuffer::clear),
            resolve<void(int, int)> (&MidiBuffer::clear)),
        "is_empty",         &MidiBuffer::isEmpty,
        "get_num_events",   &MidiBuffer::getNumEvents,
        "swap_with",        &MidiBuffer::swapWith,
        "iterator", [](const MidiBuffer& b) {
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

    lua.script (
R"(function MidiBuffer:iter()
   local iter = MidiBufferIterator.make (self)
   local msg = MidiMessage.make()
   return function()
      local ok, frame = iter:get_next_event (msg)
      if not ok then
         return nil
      else
          return msg, frame
      end
   end
end
)");

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

    lua.new_usertype<kv::PortList> ("PortList",
        sol::constructors<kv::PortList()>(),
        "add", [](kv::PortList& ports, int type, int index, int channel, 
                  const std::string& symbol, const std::string& name,
                  const bool input)
                  {
                      ports.add (type, index, channel, symbol, name, input);
                  }
    );

    // Node
    // lua.new_usertype<Node> ("Node", no_constructor,
    // );

    // // Session
    // lua.new_usertype<Session> ("Session", no_constructor,
    // );
}

}}

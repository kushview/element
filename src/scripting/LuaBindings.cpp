/*
    This file is part of Element
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

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

#include "engine/MidiPipe.h"
#include "session/Node.h"
#include "session/Session.h"
#include "session/PluginManager.h"
#include "session/CommandManager.h"
#include "Globals.h"
#include "Settings.h"

#include "sol/sol.hpp"

namespace sol {
/** Support juce::ReferenceCountedObjectPtr */
template <typename T>
struct unique_usertype_traits<ReferenceCountedObjectPtr<T>> {
    typedef T type;
    typedef ReferenceCountedObjectPtr<T> actual_type;
    static const bool value = true;
    static bool is_null (const actual_type& ptr)    { return ptr == nullptr; }
    static type* get (const actual_type& ptr)       { return ptr.get(); }
};

}

using namespace sol;

namespace Element {
namespace Lua {

static MidiBuffer::Iterator midiBufferIteratorFactory (MidiBuffer& buffer)
{
    MidiBuffer::Iterator iter (buffer);
    return std::move (iter);
}

static auto NS (state& lua, const char* name) { return lua[name].get_or_create<table>(); }

void registerUI (state& lua)
{
}

void registerModel (sol::state& lua)
{
    // Sesson
    auto session = lua.new_usertype<Session> ("Session", no_constructor,
        "get_num_graphs",           &Session::getNumGraphs,
        "get_graph",                &Session::getGraph,
        "get_active_graph",         &Session::getActiveGraph,
        "get_active_graph_index",   &Session::getActiveGraphIndex,
        "add_graph",                &Session::addGraph,
        "set_name", [](Session& self, const char* name) -> void {
            self.setName (String::fromUTF8 (name));
        },
        "get_name", [](const Session& self) -> std::string {
            return std::move (self.getName().toStdString());
        },
        "clear",                    &Session::clear
    );

    // Node
    auto node = lua.new_usertype<Node> ("Node", no_constructor,
        "is_valid",             &Node::isValid,
        "get_name",             [](const Node& self) { return std::move (self.getName().toStdString()); },
        "get_display_name",     [](const Node& self) { return std::move (self.getDisplayName().toStdString()); },
        "get_plugin_name",      [](const Node& self) { return std::move (self.getPluginName().toStdString()); },
        "set_name", [](Node& self, const char* name) -> void {
            self.setProperty (Tags::name, String::fromUTF8 (name));
        },
        "has_modified_name",    &Node::hasModifiedName,
        "get_node_id",          &Node::getNodeId,
        "get_uuid_string",      &Node::getUuidString,
        "get_uuid",             &Node::getUuid,
        "is_graph",             &Node::isGraph,
        "is_root_graph",        &Node::isRootGraph,
        "get_node_type",        &Node::getNodeType,
        "has_node_type",        &Node::hasNodeType,
        "has_editor",           &Node::hasEditor,
        "get_parent_graph",     &Node::getParentGraph,
        "is_child_of_root_graph", &Node::isChildOfRootGraph,
        "is_missing",           &Node::isMissing,
        "is_enabled",           &Node::isEnabled,
        // "get_midi_channels"
        "is_bypassed",          &Node::isBypassed,
        "is_muted",             &Node::isMuted,
        "is_muting_inputs",     &Node::isMutingInputs,
        "set_muted",            &Node::setMuted,
        "set_mute_input",       &Node::setMuteInput,
        "write_to_file",        [](const Node& node, const char* filepath) -> bool {
            if (! File::isAbsolutePath (filepath))
                return false;
            return node.writeToFile (File (filepath));
        },
        "reset_ports",          &Node::resetPorts,
        "save_plugin_state",    &Node::savePluginState,
        "restore_plugin_state", &Node::restorePluginState,
        "create_default_graph", Node::createDefaultGraph,
        "create_graph",         []() -> Node { return Node::createGraph(); }
    );
}

void registerEngine (state& lua)
{
    // Decibels
    auto db = lua["decibels"].get_or_create<table>();
    db["to_gain"]   = [](float input) { return Decibels::decibelsToGain (input); };
    db["from_gain"] = [](float input) { return Decibels::gainToDecibels (input); };

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
        call_constructor,           factories([]() { return std::move (MidiMessage()); }),
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
        "program_change",           MidiMessage::programChange,
        "is_pitch_wheel",           &MidiMessage::isPitchWheel,
        "get_pitch_wheel_value",    &MidiMessage::getPitchWheelValue,
        "pitch_wheel",              MidiMessage::pitchWheel
    );

    // MidiBuffer
    auto mb = lua.new_usertype<MidiBuffer> ("MidiBuffer", no_constructor,
        "clear", overload (
            resolve<void()> (&MidiBuffer::clear),
            resolve<void(int, int)> (&MidiBuffer::clear)),
        "is_empty",         &MidiBuffer::isEmpty,
        "get_num_events",   &MidiBuffer::getNumEvents,
        "swap_with",        &MidiBuffer::swapWith
    );

    mb["Iterator"] = lua.new_usertype<MidiBuffer::Iterator> ("MidiBuffer.Iterator", no_constructor,
        call_constructor, factories ([](MidiBuffer& buffer) {
            MidiBuffer::Iterator iter (buffer);
            return std::move (iter); 
        }),
        "set_next_sample_position", &MidiBuffer::Iterator::setNextSamplePosition,
        "get_next_event", [](MidiBuffer::Iterator& iter, MidiMessage& msg) {
            int frame = 0;
            bool ok = iter.getNextEvent (msg, frame);
            return std::tuple (ok, frame);
        }
    );

    lua.script (
R"(function MidiBuffer:iter()
   local iter = MidiBuffer.Iterator (self)
   local msg = MidiMessage()
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

void registerElement (sol::state& lua)
{


    auto e = NS (lua, "Element");
    e["plugins"] = [&lua]() -> PluginManager&
    {
        jassert(lua["__world__"].valid());
        return ((Globals&) lua["__world__"]).getPluginManager();
    };

    e["session"] = [&lua]() -> SessionPtr
    {
        jassert(lua["__world__"].valid());
        return ((Globals&) lua["__world__"]).getSession();
    };

    e["settings"] = [&lua]() -> Settings&
    {
        jassert(lua["__world__"].valid());
        return ((Globals&) lua["__world__"]).getSettings();
    };
}

void setWorld (sol::state& lua, Globals* world)
{
    if (world == nullptr)
        lua["__world__"] = nullptr;
    else
        lua["__world__"] = std::ref (*world);
}

}}

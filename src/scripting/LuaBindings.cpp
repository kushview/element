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
#include "session/CommandManager.h"
#include "session/MediaManager.h"
#include "session/Node.h"
#include "session/Session.h"
#include "session/PluginManager.h"
#include "session/Presets.h"
#include "Globals.h"
#include "Settings.h"

#include "scripting/LuaIterators.h"

#include "sol/sol.hpp"

//=============================================================================
extern int luaopen_decibels (lua_State*);

//=============================================================================
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

#define CALL(x) sol::c_call<decltype(x), x>
#define WRAP(x) sol::wrap<decltype(x), x>

using namespace sol;

namespace Element {
namespace Lua {

static auto NS (state& lua, const char* name) { return lua[name].get_or_create<table>(); }


void registerUI (state& lua)
{
}

void registerModel (sol::state& lua)
{
    auto e = NS (lua, "element");
    // Sesson
    auto session = e.new_usertype<Session> ("Session", no_constructor,
        meta_function::to_string, [](Session* self) {
            String str = "Session"; 
            if (self->getName().isNotEmpty())
                str << ": " << self->getName();
            return std::move (str.toStdString());
        },
        meta_function::length,      [](Session* self) { return self->getNumGraphs(); },
        meta_function::index,       [](Session* self, int index) {
            return isPositiveAndBelow (--index, self->getNumGraphs())
                ? std::make_shared<Node> (self->getGraph(index).getValueTree(), false)
                : std::shared_ptr<Node>();
        },
        "get_num_graphs",           &Session::getNumGraphs,
        "get_graph",                &Session::getGraph,
        "get_active_graph",         &Session::getActiveGraph,
        "get_active_graph_index",   &Session::getActiveGraphIndex,
        "add_graph",                &Session::addGraph,
        "set_name", [](Session* self, const char* name) -> void {
            self->setName (String::fromUTF8 (name));
        },
        "get_name", [](const Session& self) -> std::string {
            return std::move (self.getName().toStdString());
        },
        "clear",                    &Session::clear,

        "save_graph_state",         &Session::saveGraphState,
        "restore_graph_state",      &Session::restoreGraphState
    );

    // Node
    auto node = e.new_usertype<Node> ("Node", no_constructor,
        meta_function::to_string, [](const Node& self) -> std::string {
            String str = self.isGraph() ? "Graph" : "Node";
            if (self.getName().isNotEmpty())
                str << ": " << self.getName();
            return std::move (str.toStdString());
        },
        meta_function::length,  &Node::getNumNodes,
        meta_function::index,   [](Node* self, int index)
        {
            const auto child = self->getNode (index - 1);
            return child.isValid() ? std::make_shared<Node> (child.getValueTree(), false)
                                   : std::shared_ptr<Node>();
        },
        "to_xml_string", [](Node* self) -> std::string
        {
            auto copy = self->getValueTree().createCopy();
            Node::sanitizeRuntimeProperties (copy, true);
            return std::move (copy.toXmlString().toStdString());
        },
        "is_valid",             &Node::isValid,
        "get_name",             [](Node* self) { return std::move (self->getName().toStdString()); },
        "get_display_name",     [](Node* self) { return std::move (self->getDisplayName().toStdString()); },
        "get_plugin_name",      [](Node* self) { return std::move (self->getPluginName().toStdString()); },
        "set_name", [](Node* self, const char* name) -> void {
            self->setProperty (Tags::name, String::fromUTF8 (name));
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
        "get_num_nodes",        &Node::getNumNodes,
        "get_node",             &Node::getNode,
        "write_to_file",        [](const Node& node, const char* filepath) -> bool {
            if (! File::isAbsolutePath (filepath))
                return false;
            return node.writeToFile (File (filepath));
        },
        "reset_ports",          &Node::resetPorts,
        "save_plugin_state",    &Node::savePluginState,
        "restore_plugin_state", &Node::restorePluginState,
        "create_default_graph", overload (
            []() { return Node::createDefaultGraph(); },
            [](const char* name) { return Node::createDefaultGraph (name); }),
        "create_graph", overload (
            []() { return Node::createGraph(); },
            [](const char* name) { return Node::createGraph (name); })
    );
    e.set_function ("create_graph", []() { return Node::createGraph(); });
    e.set_function ("create_default_graph", []() { return Node::createDefaultGraph(); });
}

static void openMidi (state& lua)
{
    auto midi = NS (lua, "midi");
    
    // MidiMessage
    midi.new_usertype<MidiMessage> ("Message", no_constructor,
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
        "note_on",                  resolve<MidiMessage(int, int, uint8)> (MidiMessage::noteOn),
        "note_on_float",            resolve<MidiMessage(int, int, float)> (MidiMessage::noteOn),
        "is_note_off",              overload (&MidiMessage::isNoteOff),
        "note_off",                 resolve<MidiMessage (int, int, uint8)> (MidiMessage::noteOff),
        "note_off_float",           resolve<MidiMessage (int, int, float)> (MidiMessage::noteOff),
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
        "pitch_wheel",              MidiMessage::pitchWheel,
        "is_after_touch",           &MidiMessage::isAftertouch,
        "get_after_touch_value",    &MidiMessage::getAfterTouchValue,
        "after_touch_change",       MidiMessage::aftertouchChange,
        "is_channel_pressure",      &MidiMessage::isChannelPressure,
        "get_channel_pressure_value", &MidiMessage::getChannelPressureValue,
        "channel_pressure_change",  MidiMessage::channelPressureChange,
        "is_controller",            &MidiMessage::isController,
        "get_controller_number",    &MidiMessage::getControllerNumber,
        "get_controller_value",     &MidiMessage::getControllerValue,
        "is_controller_of_type",    &MidiMessage::isControllerOfType,
        "controller_event",         MidiMessage::controllerEvent,
        "is_all_notes_off",         &MidiMessage::isAllNotesOff,
        "is_all_sound_off",         &MidiMessage::isAllSoundOff,
        "is_reset_all_controllers", &MidiMessage::isResetAllControllers,
        "all_notes_off",            MidiMessage::allNotesOff,
        "all_sounds_off",           MidiMessage::allSoundOff,
        "all_controllers_off",      MidiMessage::allControllersOff,
        "is_meta_event",            &MidiMessage::isMetaEvent,
        "get_meta_event_type",      &MidiMessage::getMetaEventType,
        "get_meta_event_data",      &MidiMessage::getMetaEventData,
        "get_meta_event_length",    &MidiMessage::getMetaEventLength,
        "is_track_meta_event",      &MidiMessage::isTrackMetaEvent,
        "is_end_of_track_meta_event", &MidiMessage::isEndOfTrackMetaEvent,
        "end_of_track",             MidiMessage::endOfTrack,
        "is_track_name_event",      &MidiMessage::isTrackNameEvent,
        "is_text_meta_event",       &MidiMessage::isTextMetaEvent,
        "text_meta_event",          MidiMessage::textMetaEvent,
        "is_tempo_meta_event",      &MidiMessage::isTempoMetaEvent,
        "get_tempo_meta_event_tick_length",     &MidiMessage::getTempoMetaEventTickLength,
        "get_tempo_seconds_per_quarter_note",   &MidiMessage::getTempoSecondsPerQuarterNote,
        "tempo_meta_event",         MidiMessage::tempoMetaEvent,
        "is_time_signature_meta_event", &MidiMessage::isTimeSignatureMetaEvent,
        "get_time_signature_info",      [](const MidiMessage& self) {
            int numerator = 0, denominator = 0;
            self.getTimeSignatureInfo (numerator, denominator);
            return std::make_tuple (numerator, denominator);
        },
        "time_signature_meta_event",    MidiMessage::timeSignatureMetaEvent,
        "is_key_signature_meta_event",  &MidiMessage::isKeySignatureMetaEvent,
        "get_key_signature_number_of_sharps_or_flats", &MidiMessage::getKeySignatureNumberOfSharpsOrFlats,
        "is_key_signature_major_key",   &MidiMessage::isKeySignatureMetaEvent,
        "key_signature_meta_event",     MidiMessage::keySignatureMetaEvent,
        "is_midi_channel_meta_event",   &MidiMessage::isMidiChannelMetaEvent,
        "get_midi_channel_meta_event_channel", &MidiMessage::getMidiChannelMetaEventChannel,
        "midi_channel_meta_event",      MidiMessage::midiChannelMetaEvent,
        "is_active_sense",              &MidiMessage::isActiveSense,
        "is_midi_start",                &MidiMessage::isMidiStart,
        "midi_start",                   MidiMessage::midiStart,
        "is_midi_continue",             &MidiMessage::isMidiContinue,
        "midi_continue",                MidiMessage::midiContinue,
        "is_midi_stop",                 &MidiMessage::isMidiStop,
        "midi_stop",                    MidiMessage::midiStop,  
        "is_midi_clock",                &MidiMessage::isMidiClock,
        "midi_clock",                   MidiMessage::midiClock,
        "is_song_position_pointer",     &MidiMessage::isSongPositionPointer,
        "get_song_position_pointer_midi_beat", &MidiMessage::getSongPositionPointerMidiBeat,
        "song_position_pointer",        MidiMessage::songPositionPointer,
        "is_quarter_frame",             &MidiMessage::isQuarterFrame,
        "get_quarter_frame_sequence_number", &MidiMessage::getQuarterFrameSequenceNumber,
        "get_quarter_frame_value",      &MidiMessage::getQuarterFrameValue,
        "quarter_frame",                MidiMessage::quarterFrame,
        "is_full_frame",                &MidiMessage::isFullFrame,
        "get_full_frame_parameters", [](const MidiMessage& self) {
            int hours, minutes, seconds, frames;
            MidiMessage::SmpteTimecodeType stype;
            self.getFullFrameParameters (hours, minutes, seconds, frames, stype);
            return std::make_tuple (hours, minutes, seconds, frames, static_cast<int> (stype));
        },
        "full_frame",                       MidiMessage::fullFrame,
        "is_midi_machine_control_message",  &MidiMessage::isMidiMachineControlMessage,
        "get_midi_machine_control_command", &MidiMessage::getMidiMachineControlCommand,
        "midi_machine_control_command",     MidiMessage::midiMachineControlCommand,
        "is_midi_machine_control_goto",     [](const MidiMessage& self) {
            int hours = 0, minutes = 0, seconds = 0, frames = 0;
            bool ok = self.isMidiMachineControlGoto (hours, minutes, seconds, frames);
            return std::make_tuple (ok, hours, minutes, seconds, frames);
        },
        "midi_machine_control_goto",        MidiMessage::midiMachineControlGoto,
        "master_volume",                    MidiMessage::masterVolume,
        "create_sysex_message",             MidiMessage::createSysExMessage,
        "read_variable_length_val",         [](const uint8* data) {
            int nbytes = 0;
            int value = MidiMessage::readVariableLengthVal (data, nbytes);
            return std::make_tuple (nbytes, value);
        },
        "get_message_length_from_first_byte", MidiMessage::getMessageLengthFromFirstByte,
        "get_midi_note_name",               MidiMessage::getMidiNoteName,
        "get_midi_note_in_hertz",           MidiMessage::getMidiNoteInHertz,
        "is_midi_note_black",               MidiMessage::isMidiNoteBlack,
        "get_gm_instrument_name",           MidiMessage::getGMInstrumentName,
        "get_gm_instrument_bank_name",      MidiMessage::getGMInstrumentBankName,
        "get_rhythm_instrument_name",       MidiMessage::getRhythmInstrumentName,
        "get_controller_name",              MidiMessage::getControllerName,
        "float_value_to_midi_byte",         MidiMessage::floatValueToMidiByte,
        "pitchbend_to_pitchwheel_pos",      MidiMessage::pitchbendToPitchwheelPos
    );

    // MidiBuffer
    midi.new_usertype<MidiBuffer> ("Buffer", no_constructor,
        call_constructor, factories (
            []() { return std::move (MidiBuffer()); },
            [](MidiMessage* message) { return std::move (MidiBuffer (*message)); }),
        meta_method::length,        &MidiBuffer::getNumEvents,
        "empty",                    readonly_property (&MidiBuffer::isEmpty),
        "first",                    readonly_property (&MidiBuffer::getFirstEventTime),
        "last",                     readonly_property (&MidiBuffer::getLastEventTime),
        "data",                     readonly_property (&MidiBuffer::data),
        "swap",                     &MidiBuffer::swapWith,
        "reserve",                  &MidiBuffer::ensureSize,
        "clear", overload (
            resolve<void()> (&MidiBuffer::clear),               
            resolve<void(int, int)> (&MidiBuffer::clear)),        
        "add", overload (
            resolve<void(const MidiMessage&, int)> (&MidiBuffer::addEvent),
            resolve<void(const void*, int, int)> (&MidiBuffer::addEvent),
            resolve<void(const MidiBuffer&, int, int, int)> (&MidiBuffer::addEvents)),              
        "foreach", [](MidiBuffer* self) {
            MidiBufferForeach fe (*self);
            return std::move (fe);
        }
    );

    midi.new_usertype<MidiBufferForeach> ("Iterator", no_constructor,
        meta_method::to_string, [](MidiBufferForeach*) { return "midi.Iterator"; }
    );

    midi.set_function ("noteon",    [](int c, int n, uint8 v) { return MidiMessage::noteOn (c, n, v); });
    midi.set_function ("noteonf",   [](int c, int n, float v) { return MidiMessage::noteOn (c, n, v); });
    midi.set_function ("noteoff",   [](int channel, int note) { return MidiMessage::noteOff (channel, note); });
    midi.set_function ("noteoffv",  [](int channel, int note, uint8_t velocity) { return MidiMessage::noteOff (channel, note, velocity); });
    midi.set_function ("noteoffvf", [](int channel, int note, float velocity)   { return MidiMessage::noteOff (channel, note, velocity); });
}

static void openDecibels (state& lua)
{
    luaL_requiref (lua, "decibels", luaopen_decibels, 0);
    lua_pop (lua, lua_gettop (lua));
}

template<typename T>
static auto addRange (state_view& view, const char* name)
{
    using R = Range<T>;
    return view.new_usertype<R> (name, no_constructor,
        call_constructor, factories (
            []() { return R(); },
            [] (T start, T end) { return R (start, end); }
        ),
        "empty",            readonly_property (&R::isEmpty),
        "start",            property (&R::getStart,  &R::setStart),
        "length",           property (&R::getLength, &R::setLength),
        "end",              property (&R::getEnd,    &R::setEnd),
        "clip",             &R::clipValue,
        "contains",         [](R* self, R* other) { return self->contains (*other); },
        "intersects",       [](R* self, R* other) { return self->intersects (*other); },
        "expanded",         &R::expanded
    );
}

template<typename T>
static auto addRectangle (state& lua, const char* ns, const char* name)
{
    using R = Rectangle<T>;
    auto view = NS (lua, ns);
    return view.new_usertype<R> (name, no_constructor,
        call_constructor, factories (
            []() { return R(); },
            [] (T w, T h) { return R (w, h); },
            [] (T x, T y, T w, T h) { return R (x, y, w, h); }
        ),
        meta_method::to_string, [](R* self) {
            return std::move (self->toString().toStdString());
        },
        "empty",            readonly_property (&R::isEmpty),
        "x",                property (&R::getX,  &R::setX),
        "y",                property (&R::getY,  &R::setY),
        "w",                property (&R::getWidth, &R::setWidth),
        "h",                property (&R::getHeight, &R::setHeight)
    );
}

static void openJUCE (state& lua)
{
    addRange<float>     (lua, "Range");
    addRange<int>       (lua, "Span");
    addRectangle<float> (lua, "element", "Rectangle");
    addRectangle<int>   (lua, "ui", "Bounds");
    
    // AudioBuffer
    lua.new_usertype<AudioSampleBuffer> ("AudioBuffer", no_constructor,
        "cleared",      readonly_property (&AudioSampleBuffer::hasBeenCleared),
        "nchannels",    readonly_property (&AudioSampleBuffer::getNumChannels),
        "nframes",      readonly_property (&AudioSampleBuffer::getNumSamples),
        "resize", overload (
            [](AudioSampleBuffer& self, int nc, int ns) { self.setSize (nc, ns); },
            [](AudioSampleBuffer& self, int nc, int ns, bool keep) { self.setSize (nc, ns, keep); },
            [](AudioSampleBuffer& self, int nc, int ns, bool keep, bool clear) { self.setSize (nc, ns, keep, clear); },
            [](AudioSampleBuffer& self, int nc, int ns, bool keep, bool clear, bool avoid) { self.setSize (nc, ns, keep, clear, avoid); }),
        "duplicate", overload (
            [](AudioSampleBuffer& self, const AudioSampleBuffer& other) { self.makeCopyOf (other); },
            [](AudioSampleBuffer& self, const AudioSampleBuffer& other, bool avoidReallocate) { self.makeCopyOf (other, avoidReallocate); }),
        "clear", overload (
            resolve<void()> (&AudioSampleBuffer::clear),
            resolve<void(int,int)> (&AudioSampleBuffer::clear),
            resolve<void(int,int,int)> (&AudioSampleBuffer::clear)),
        "get_sample",       &AudioSampleBuffer::getSample,
        "set_sample",       CALL(&AudioSampleBuffer::setSample),
        "add_sample",       &AudioSampleBuffer::addSample,
        "apply_gain", overload (
            resolve<void(int,int,int,float)> (&AudioSampleBuffer::applyGain),
            resolve<void(int,int,float)> (&AudioSampleBuffer::applyGain),
            resolve<void(float)> (&AudioSampleBuffer::applyGain)),
        "apply_gain_ramp", overload (
            resolve<void(int,int,int,float,float)> (&AudioSampleBuffer::applyGainRamp),
            resolve<void(int,int,float,float)> (&AudioSampleBuffer::applyGainRamp)),
        "add_from", overload (
            [](AudioSampleBuffer& self, int dc, int dss, AudioSampleBuffer& src, int sc, int sss, int ns) {
                self.addFrom (dc, dss, src, sc, sss, ns);
            },
            [](AudioSampleBuffer& self, int dc, int dss, AudioSampleBuffer& src, int sc, int sss, int ns, float gain) {
                self.addFrom (dc, dss, src, sc, sss, ns, gain);
            }),
        "add_from_with_ramp",   &AudioSampleBuffer::addFromWithRamp,
        "copy_from", overload (
            resolve<void(int,int,const AudioSampleBuffer&,int,int,int)> (&AudioSampleBuffer::copyFrom),
            resolve<void(int,int,const float*, int)> (&AudioSampleBuffer::copyFrom),
            resolve<void(int,int,const float*, int, float)> (&AudioSampleBuffer::copyFrom)),
        "copy_from_with_ramp",  &AudioSampleBuffer::copyFromWithRamp,
        "find_min_max",         &AudioSampleBuffer::findMinMax,
        "magnitude", overload (
            [](const AudioSampleBuffer& self, int c, int s, int n) { return self.getMagnitude (c, s, n); },
            [](const AudioSampleBuffer& self, int s, int n) { return self.getMagnitude (s, n); }),
        "rms",                  &AudioSampleBuffer::getRMSLevel,
        "reverse",overload (
            [](const AudioSampleBuffer& self, int c, int s, int n) { return self.reverse (c, s, n); },
            [](const AudioSampleBuffer& self, int s, int n) { return self.reverse (s, n); })
    );
    
    auto midi = NS (lua, "midi");

}

static void openEngine (state& lua)
{
    auto midi = NS (lua, "midi");
    auto e    = NS (lua, "element");
    auto kv   = NS (lua, "kv");

    // MidiPipe
    midi.new_usertype<MidiPipe> ("Pipe", no_constructor,
        meta_method::to_string, [](MidiPipe*) { return "midi.Pipe"; },
        meta_method::index, [](MidiPipe* self, int index) -> MidiBuffer* {
            return isPositiveAndBelow(--index, self->getNumBuffers())
                ? self->getWriteBuffer(index) : nullptr; 
        },
        meta_method::length,    &MidiPipe::getNumBuffers,
        "get_num_buffers",      &MidiPipe::getNumBuffers,
        "get_read_buffer",      &MidiPipe::getReadBuffer,
        "get_write_buffer",     &MidiPipe::getWriteBuffer,
        "buffer",               &MidiPipe::getWriteBuffer,
        "clear", overload (
            resolve<void()> (&MidiPipe::clear),
            resolve<void(int, int)> (&MidiPipe::clear),
            resolve<void(int, int, int)> (&MidiPipe::clear))
    );

    kv.new_usertype<kv::PortType> ("PortType", no_constructor,
        meta_method::to_string, [](PortType*) { return "kv.PortType"; }
    );

    // PortList
    kv.new_usertype<kv::PortList> ("PortList",
        sol::constructors<kv::PortList()>(),
        meta_method::to_string, [](MidiPipe*) { return "kv.PortList"; },
        "add", [](kv::PortList& ports, int type, int index, int channel,
                  const char* symbol, const char* name,
                  const bool input)
                  {
                      ports.add (type, index, channel, symbol, name, input);
                  }
    );
}

void registerEngine (state& lua)
{
    openMidi (lua);
    openDecibels (lua);
    openJUCE (lua);
    openEngine (lua);
}

void registerElement (state& lua)
{
    auto e = NS (lua, "element");
    
    e.new_usertype<Globals> ("World", no_constructor,
        "audio_engine",     &Globals::getAudioEngine,
        "commands",         &Globals::getCommandManager,
        "devices",          &Globals::getDeviceManager,
        "mappings",         &Globals::getMappingEngine,
        "media",            &Globals::getMediaManager,
        "midi_engine",      &Globals::getMidiEngine,
        "plugins",          &Globals::getPluginManager,
        "presets",          &Globals::getPresetCollection,
        "session",          &Globals::getSession,
        "settings",         &Globals::getSettings
    );
}

void setWorld (state& lua, Globals* world)
{
    auto e = NS (lua, "element");
    
    if (world != nullptr)
    {
        e.set_function ("world",            [world]() -> Globals&           { return *world; });
        e.set_function ("audio_engine",     [world]() -> AudioEnginePtr     { return world->getAudioEngine(); });
        e.set_function ("commands",         [world]() -> CommandManager&    { return world->getCommandManager(); });
        e.set_function ("devices",          [world]() -> DeviceManager&     { return world->getDeviceManager(); });
        e.set_function ("mapping_engine",   [world]() -> MappingEngine&     { return world->getMappingEngine(); });
        e.set_function ("media",            [world]() -> MediaManager&      { return world->getMediaManager(); });
        e.set_function ("midi_engine",      [world]() -> MidiEngine&        { return world->getMidiEngine(); });
        e.set_function ("plugins",          [world]() -> PluginManager&     { return world->getPluginManager(); });
        e.set_function ("presets",          [world]() -> PresetCollection&  { return world->getPresetCollection(); });
        e.set_function ("session",          [world]() -> SessionPtr         { return world->getSession(); });
        e.set_function ("settings",         [world]() -> Settings&          { return world->getSettings(); });
    }
    else
    {
        for (const auto& f : StringArray{ "world", "audio_engine", "commands", "devices",
                                          "mapping_engine", "media", "midi_engine", "plugins", 
                                          "presets", "session", "settings" })
        {
            e.set_function (f.toRawUTF8(), []() { return sol::lua_nil; });
        }
    }
}

}}

// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cmath>

#include "el/factories.hpp"
#include <element/midipipe.hpp>
#include "scripting/dspscript.hpp"
#include "scripting/scriptloader.hpp"
#include "scripting/bindings.hpp"
#include <element/processor.hpp>

using namespace element;
using namespace juce;

namespace element {

//==============================================================================

/// Time information for DSP Scripts.
// An object of this type is passed to a DSP Script's process function
// conveying timing information.
// @classmod el.DSPScriptPosition
// @pragma nostrip
struct DSPScriptPosition
{
    using PositionType = juce::AudioPlayHead::PositionInfo;
    bool valid = false;
    bool playing { false },
        looping { false },
        recording { false };

    lua_Integer frame = 0;
    lua_Number seconds = 0.0;

    lua_Number bpm = 0.0;
    lua_Integer beatsPerBar = 4, beatUnit = 4;
    lua_Integer bar = 0;
    lua_Number beat = 0.0;

    void update (Optional<PositionType> pos)
    {
        valid = pos.hasValue();
        if (! valid)
            return;

        playing = pos->getIsPlaying();
        looping = pos->getIsLooping();
        recording = pos->getIsRecording();

        if (auto f = pos->getTimeInSamples())
            frame = static_cast<lua_Integer> (*f);

        if (auto s = pos->getTimeInSeconds())
            seconds = *s;

        if (auto b = pos->getBpm())
            bpm = *b;

        if (auto ts = pos->getTimeSignature())
        {
            beatsPerBar = ts->numerator;
            beatUnit = ts->denominator;
        }

        if (auto b = pos->getBarCount())
            bar = *b;
        if (auto b = pos->getPpqPosition())
            beat = *b;
    }

    static DSPScriptPosition** create (lua_State* L)
    {
        auto pos = lua::new_userdata<DSPScriptPosition> (L, "el.DSPScriptPosition");
        return pos;
    }

    static int gc (lua_State* L) { return 0; }

    static int _valid (lua_State* L)
    {
        auto self = *(DSPScriptPosition**) lua_touserdata (L, 1);
        lua_pushboolean (L, self->valid);
        return 1;
    }

    static int _playing (lua_State* L)
    {
        auto self = *(DSPScriptPosition**) lua_touserdata (L, 1);
        lua_pushboolean (L, self->playing);
        return 1;
    }

    static int _recording (lua_State* L)
    {
        auto self = *(DSPScriptPosition**) lua_touserdata (L, 1);
        lua_pushboolean (L, self->recording);
        return 1;
    }

    static int _looping (lua_State* L)
    {
        auto self = *(DSPScriptPosition**) lua_touserdata (L, 1);
        lua_pushboolean (L, self->looping);
        return 1;
    }

    static int _frame (lua_State* L)
    {
        auto self = *(DSPScriptPosition**) lua_touserdata (L, 1);
        lua_pushinteger (L, self->frame);
        return 1;
    }

    static int _seconds (lua_State* L)
    {
        auto self = *(DSPScriptPosition**) lua_touserdata (L, 1);
        lua_pushinteger (L, self->seconds);
        return 1;
    }

    static int _bpm (lua_State* L)
    {
        auto self = *(DSPScriptPosition**) lua_touserdata (L, 1);
        lua_pushinteger (L, self->bpm);
        return 1;
    }

    static int _beatsPerBar (lua_State* L)
    {
        auto self = *(DSPScriptPosition**) lua_touserdata (L, 1);
        lua_pushinteger (L, self->beatsPerBar);
        return 1;
    }

    static int _beatUnit (lua_State* L)
    {
        auto self = *(DSPScriptPosition**) lua_touserdata (L, 1);
        lua_pushinteger (L, self->beatUnit);
        return 1;
    }

    static int _bar (lua_State* L)
    {
        auto self = *(DSPScriptPosition**) lua_touserdata (L, 1);
        lua_pushinteger (L, self->bar);
        return 1;
    }

    static int _beat (lua_State* L)
    {
        auto self = *(DSPScriptPosition**) lua_touserdata (L, 1);
        lua_pushinteger (L, self->beat);
        return 1;
    }

    static const luaL_Reg* methods()
    {
        static const luaL_Reg sPositionMethods[] = {
            { "__gc", DSPScriptPosition::gc },

            /// True if playing.
            // @within Methods
            // @function DSPScriptPosition:playing
            { "playing", DSPScriptPosition::_playing },

            /// True if recording.
            // @within Methods
            // @function DSPScriptPosition:recording
            { "recording", DSPScriptPosition::_recording },

            /// True if looping
            // @within Methods
            // @function DSPScriptPosition:looping
            { "looping", DSPScriptPosition::_looping },

            /// True if this position is valid.
            // All other methods are not reliable if this returns false.
            // @within Methods
            // @function DSPScriptPosition:valid
            { "valid", DSPScriptPosition::_valid },

            /// Time in audio samples.
            // @within Methods
            // @function DSPScriptPosition:frame
            // @return The frame as an Integer
            { "frame", DSPScriptPosition::_frame },

            /// Time in seconds.
            // @within Methods
            // @function DSPScriptPosition:seconds
            // @return Time in seconds as a number
            { "seconds", DSPScriptPosition::_seconds },

            /// The current BPM.
            // @within Methods
            // @function DSPScriptPosition:bpm
            // @return The bpm
            { "bpm", DSPScriptPosition::_bpm },

            /// Beats per bar e.g. the numerator in a time signature
            // @within Methods
            // @function DSPScriptPosition:beatsPerBar
            // @return The time signature numerator.
            { "beatsPerBar", DSPScriptPosition::_beatsPerBar },

            /// Denominator of a the time signature.
            // @within Methods
            // @function DSPScriptPosition:beatUnit
            // @return The time signature denominator
            { "beatUnit", DSPScriptPosition::_beatUnit },

            /// The current bar.
            // @within Methods
            // @function DSPScriptPosition:bar
            // @return The bar
            { "bar", DSPScriptPosition::_bar },

            /// The current beat in terms of quarter note.
            // @within Methods
            // @function DSPScriptPosition:beat
            // @return The beat
            { "beat", DSPScriptPosition::_beat },

            { nullptr, nullptr }
        };
        return sPositionMethods;
    }

    static int loadModule (lua_State* L)
    {
        if (luaL_newmetatable (L, "el.DSPScriptPosition"))
        {
            lua_pushvalue (L, -1); /* duplicate the metatable */
            lua_setfield (L, -2, "__index"); /* mt.__index = mt */
            luaL_setfuncs (L, methods(), 0);
            lua_pop (L, 1);
        }

        if (luaL_newmetatable (L, "el.DSPScriptPositionClass"))
        {
            // lua_pushcfunction (L, midipipe_new); /* push audio_new function */
            // lua_setfield (L, -2, "__call"); /* mt.__call = audio_new */
            lua_pop (L, 1);
        }

        lua_newtable (L);
        luaL_setmetatable (L, "el.DSPScriptPositionClass");
        // lua_pushcfunction (L, DSPScriptPosition::create);
        // lua_setfield (L, -2, "new");
        return 1;
    }
};

//==============================================================================
class DSPScript::Parameter : public RangedParameter,
                             public element::Parameter::Listener
{
public:
    Parameter (DSPScript* c, const PortDescription& port)
        : RangedParameter (port),
          info (port),
          ctx (c)
    {
        const auto sp = getPort();
        set (sp.defaultValue);
        addListener (this);
    }

    ~Parameter() override
    {
        unlink();
    }

    String getLabel() const override { return {}; }

    void unlink()
    {
        removeListener (this);
        info = {};
        ctx = nullptr;
    }

    void controlValueChanged (int index, float value) override
    {
        if (! _block && ctx != nullptr)
        { // index may not be set so use port channel.
            ctx->setParameter (getPortChannel(), convertFrom0to1 (value), info.input);
        }
    }

    void controlTouched (int parameterIndex, bool gestureIsStarting) override {}

    // update the value notifiying listeners, but not this parameter.
    void update (float value)
    {
        _block = true;
        set (value);
        _block = false;
    }

private:
    PortDescription info;
    DSPScript* ctx { nullptr };
    bool _block = false;
};

//==============================================================================
DSPScript::DSPScript (sol::table tbl)
    : DSP (tbl)
{
    bool ok = DSP.valid();
    if (ok)
    {
        L = DSP.lua_state();
        ok = L != nullptr;
    }

    if (ok)
    {
        try
        {
            DSPScriptPosition::loadModule (L);
            lua_pop (L, 1);

            sol::state_view lua (L);
            auto result = lua.safe_script (R"(
                require ('el.audio')
                require ('el.midi')
                require ('el.AudioBuffer')
                require ('el.MidiBuffer')
                require ('el.MidiMessage')
                require ('el.MidiPipe')
            )");
            ok = result.status() == sol::call_status::ok;

            switch (result.status())
            {
                case sol::call_status::file:
                    DBG ("DSPScript: file error");
                    break;
                case sol::call_status::gc:
                    DBG ("DSPScript: gc error");
                    break;
                case sol::call_status::handler:
                    DBG ("DSPScript: handler error");
                    break;
                case sol::call_status::memory:
                    DBG ("DSPScript: memory error");
                    break;
                case sol::call_status::runtime:
                    DBG ("DSPScript: runtime error");
                    break;
                case sol::call_status::syntax:
                    DBG ("DSPScript: syntax error");
                    break;
                case sol::call_status::yielded:
                    DBG ("DSPScript: yielded error");
                    break;
                case sol::call_status::ok:
                    break;
            }
        } catch (const sol::error& e)
        {
            DBG (e.what());
            ok = false;
        }
    }

    if (ok)
    {
        processFunc = DSP["process"];
        processRef = processFunc.registry_index();
        ok = processRef != LUA_REFNIL && processRef != LUA_NOREF;
    }

    if (ok)
    {
        audio = lua::new_userdata<AudioBuffer<float>> (
            L, EL_MT_AUDIO_BUFFER_32);
        audioRef = luaL_ref (L, LUA_REGISTRYINDEX);
        ok = audioRef != LUA_REFNIL && audioRef != LUA_NOREF;
    }

    if (ok)
    {
        midi = LuaMidiPipe::create (L, 4);
        midiRef = luaL_ref (L, LUA_REGISTRYINDEX);
        ok = midiRef != LUA_REFNIL && midiRef != LUA_NOREF;
    }

    if (ok)
    {
        addAudioMidiPorts();
        addParameterPorts();
    }

    if (ok)
    {
        sol::state_view view (L);
        auto tmp = view.create_table();
        tmp["params"] = &paramData;
        paramsUserData = tmp["params"];
        ok = paramsUserData.valid();
    }

    if (ok)
    {
        sol::state_view view (L);
        auto tmp = view.create_table();
        tmp["controls"] = &controlData;
        controlsUserData = tmp["controls"];
        ok = controlsUserData.valid();
    }

    if (ok)
    {
        position = DSPScriptPosition::create (L);
        positionRef = luaL_ref (L, LUA_REGISTRYINDEX);
        ok = positionRef != LUA_REFNIL && positionRef != LUA_NOREF;
    }

    loaded = ok;
    if (! loaded)
    {
        deref();
    }
}

DSPScript::~DSPScript()
{
    unlinkParams();
    deref();
}

template <typename... Args>
bool DSPScript::callOptional (const char* name, Args&&... args)
{
    sol::protected_function f = DSP[name];
    if (! f.valid())
        return true;

    auto result = f (std::forward<Args> (args)...);
    if (result.valid())
        return true;

    sol::error error = result;
    lastError = error.what();
    std::clog << "[dsp script] " << name << ": " << lastError.toStdString() << std::endl;
    return false;
}

bool DSPScript::init() { return callOptional ("init"); }
bool DSPScript::prepare (double rate, int block) { return callOptional ("prepare", rate, block); }
bool DSPScript::release() { return callOptional ("release"); }

Result DSPScript::validate (const String& script)
{
    if (script.isEmpty())
        return Result::fail ("script contains no code");

    sol::state state;
    element::Lua::initializeState (state);
    ScriptLoader loader (state.lua_state(), script);

    if (loader.hasError())
        return Result::fail (loader.getErrorMessage());

    auto descriptor = loader.call();
    if (loader.hasError())
        return Result::fail (loader.getErrorMessage());
    if (! descriptor.valid() || descriptor.get_type() != sol::type::table)
        return Result::fail ("script did not return a table");

    auto dsp = std::make_unique<DSPScript> (descriptor.as<sol::table>());
    if (! dsp->isValid())
        return Result::fail ("could not instantiate script");

    try
    {
        const double rate = 44100.0;
        const int block = 512;
        const int cycles = 4;

        PortList ports;
        dsp->getPorts (ports);
        const int nchans = jmax (1, ports.size (PortType::Audio, true), ports.size (PortType::Audio, false));
        const int nmidi = jmax (ports.size (PortType::Midi, true), ports.size (PortType::Midi, false));

        AudioSampleBuffer audio (nchans, block);
        OwnedArray<MidiBuffer> midiBuffers;
        Array<int> midiChannels;
        for (int i = 0; i < nmidi; ++i)
        {
            midiBuffers.add (new MidiBuffer());
            midiChannels.add (i);
        }
        MidiPipe midi (midiBuffers, midiChannels);

        if (! dsp->init())
            return Result::fail ("init: " + dsp->getLastError());
        if (! dsp->prepare (rate, block))
            return Result::fail ("prepare: " + dsp->getLastError());

        for (int cycle = 0; cycle < cycles; ++cycle)
        {
            // A quiet test tone so gain-style scripts have something to shape.
            for (int c = 0; c < nchans; ++c)
                for (int f = 0; f < block; ++f)
                    audio.setSample (c, f, 0.25f * std::sin (float (f) * 0.05f));

            for (auto* buffer : midiBuffers)
            {
                buffer->clear();
                buffer->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
                buffer->addEvent (MidiMessage::noteOff (1, 60), block / 2);
            }

            dsp->process (audio, midi);
            if (! dsp->isValid())
                return Result::fail ("process: " + dsp->getLastError());

            for (int c = 0; c < audio.getNumChannels(); ++c)
                for (int f = 0; f < block; ++f)
                    if (! std::isfinite (audio.getSample (c, f)))
                        return Result::fail ("process produced non-finite audio samples");
        }

        if (! dsp->release())
            return Result::fail ("release: " + dsp->getLastError());
    } catch (const std::exception& e)
    {
        return Result::fail (e.what());
    }

    return Result::ok();
}

void DSPScript::getPorts (PortList& out)
{
    for (const auto* port : ports.getPorts())
        out.add (new PortDescription (*port));
}

void DSPScript::process (AudioSampleBuffer& a, MidiPipe& m)
{
    if (! loaded)
        return;

    const int top = lua_gettop (L);

    if (lua_rawgeti (L, LUA_REGISTRYINDEX, processRef) == LUA_TFUNCTION
        && lua_rawgeti (L, LUA_REGISTRYINDEX, audioRef) == LUA_TUSERDATA
        && lua_rawgeti (L, LUA_REGISTRYINDEX, midiRef) == LUA_TUSERDATA
        && lua_rawgeti (L, LUA_REGISTRYINDEX, paramsUserData.registry_index()) == LUA_TUSERDATA
        && lua_rawgeti (L, LUA_REGISTRYINDEX, controlsUserData.registry_index()) == LUA_TUSERDATA
        && lua_rawgeti (L, LUA_REGISTRYINDEX, positionRef) == LUA_TUSERDATA)
    {
        (*audio)->setDataToReferTo (a.getArrayOfWritePointers(),
                                    a.getNumChannels(),
                                    a.getNumSamples());
        (*midi)->swapWith (m);

        if (playhead != nullptr)
            (*position)->update (playhead->getPosition());

        // Lua is built as C: an error inside an unprotected lua_call has no
        // handler to unwind to and aborts the process. A protected call keeps
        // the audio thread alive; the script is disabled instead. The String
        // assignment only happens on that error path.
        if (lua_pcall (L, 5, 0, 0) != LUA_OK)
        {
            lastError = lua_tostring (L, -1);
            std::clog << "[dsp script] process: " << lastError.toStdString() << std::endl;
            loaded = false;
        }

        (*midi)->swapWith (m);

        for (int ci = outParams.size(); --ci >= 0;)
            outParams.getUnchecked (ci)->update (controlData[ci]);
    }
    else
    {
        DBG ("didn't get render function in callback");
    }

    lua_settop (L, top);
}

void DSPScript::save (MemoryBlock& out)
{
    ValueTree state ("DSP");
    MemoryBlock block;

    block.reset();
    getParameterData (block, true);
    if (block.getSize() > 0)
        state.setProperty ("params", block, nullptr);
    block.reset();
    getParameterData (block, false);
    if (block.getSize() > 0)
        state.setProperty ("controls", block, nullptr);

    sol::function save = DSP["save"];

    if (save.valid())
    {
        sol::state_view lua (L);
        sol::environment env (lua, sol::create, lua.globals());
        try
        {
            env["dsp_script_save"] = save;
            auto result = lua.safe_script (R"(
                local tf = io.tmpfile()
                local oo = io.output()
                io.output (tf);
                dsp_script_save()
                tf:seek ('set', 0)
                local data = tf:read ("*a")
                io.close()
                io.output (oo);
                dsp_script_save = nil
                return data
            )",
                                           env);

            if (result.valid())
            {
                sol::object data = result;
                if (data.is<const char*>())
                {
                    block.reset();
                    MemoryOutputStream mo (block, false);
                    mo.write (data.as<const char*>(), strlen (data.as<const char*>()));
                }
            }
        } catch (const std::exception& e)
        {
            DBG ("[element] " << e.what());
        }
        lua.collect_garbage();
    }

    MemoryOutputStream mo (out, false);
    {
        GZIPCompressorOutputStream gz (mo);
        state.writeToStream (gz);
    }
}

void DSPScript::restore (const void* d, size_t s)
{
    const auto state = ValueTree::readFromGZIPData (d, s);
    if (! state.isValid())
        return;

    const var& params = state.getProperty ("params");
    if (params.isBinaryData())
    {
        setParameterData (*params.getBinaryData(), true);
        for (auto* const param : inParams)
        {
            const auto port = param->getPort();
            param->update (paramData[port.channel]);
        }
    }

    const var& controls = state.getProperty ("controls");
    if (controls.isBinaryData())
    {
        setParameterData (*controls.getBinaryData(), false);
        for (auto* const param : outParams)
        {
            const auto port = param->getPort();
            param->update (controlData[port.channel]);
        }
    }

    const var& data = state.getProperty ("data");
    sol::function restore = DSP["restore"];
    if (! restore.valid() || ! data.isBinaryData())
        return;

    try
    {
        sol::state_view lua (L);
        sol::environment env (lua, sol::create, lua.globals());
        sol::userdata ud = lua["io"]["tmpfile"]();
        luaL_Stream* const stream = (luaL_Stream*) ud.pointer();

        fwrite (data.getBinaryData()->getData(), 1, data.getBinaryData()->getSize(), stream->f);
        rewind (stream->f);

        env["__state_data__"] = ud;
        env["dsp_script_restore"] = restore;
        lua.safe_script (R"(
            local oi = io.input()
            io.input (__state_data__)
            dsp_script_restore()
            io.input (oi)
            __state_data__:close()
            __state_data__ = nil
            dsp_script_restore = nil
        )",
                         env);

        lua.collect_garbage();
    } catch (const std::exception& e)
    {
        DBG ("[element] " << e.what());
    }
}

void DSPScript::setParameter (int index, float value, bool input)
{
    auto data = input ? paramData : controlData;
    data[index] = value;
}

void DSPScript::copyParameterValues (const DSPScript& o)
{
    for (int i = jmin (numParams, o.numParams); --i >= 0;)
        paramData[i] = o.paramData[i];
    for (int i = jmin (numControls, o.numControls); --i >= 0;)
        controlData[i] = o.controlData[i];
}

void DSPScript::getParameterData (MemoryBlock& block, bool inputs)
{
    auto data = inputs ? paramData : controlData;
    auto size = inputs ? numParams : numControls;
    block.append (data, sizeof (float) * static_cast<size_t> (size));
}

void DSPScript::setParameterData (MemoryBlock& block, bool inputs)
{
    jassert (block.getSize() % sizeof (float) == 0);
    jassert (block.getSize() < sizeof (float) * maxParams);
    auto data = inputs ? paramData : controlData;
    memcpy (data, block.getData(), block.getSize());
}

String DSPScript::getUI() const
{
    if (! DSP.valid())
        return {};
    if (DSP["ui"].get_type() == sol::type::string)
        return DSP["ui"].get<std::string>();
    return {};
}

void DSPScript::deref()
{
    loaded = false;

    audio = nullptr;
    luaL_unref (L, LUA_REGISTRYINDEX, audioRef);
    audioRef = LUA_REFNIL;

    midi = nullptr;
    luaL_unref (L, LUA_REGISTRYINDEX, midiRef);
    midiRef = LUA_REFNIL;

    position = nullptr;
    luaL_unref (L, LUA_REGISTRYINDEX, positionRef);
    positionRef = LUA_REFNIL;
}

void DSPScript::addAudioMidiPorts()
{
    sol::function f = DSP["layout"];
    if (! f.valid())
        return;

    try
    {
        int numAudioIn = 0, numAudioOut = 0,
            numMidiIn = 0, numMidiOut = 0;

        sol::table layout = f();
        if (layout.size() > 0)
            layout = layout[1];

        sol::table audio = layout["audio"].get_or_create<sol::table>();
        numAudioIn = audio[1].get_or (0);
        numAudioOut = audio[2].get_or (0);
        sol::table midi = layout["midi"].get_or_create<sol::table>();
        numMidiIn = juce::jlimit (0, 64, (int) midi[1].get_or (0));
        numMidiOut = juce::jlimit (0, 64, (int) midi[2].get_or (0));

        int index = ports.size();
        int channel = 0;

        for (int i = 0; i < numAudioIn; ++i)
        {
            String slug = "in_";
            slug << (i + 1);
            String name = "In ";
            name << (i + 1);
            ports.add (PortType::Audio, index++, channel++, slug, name, true);
        }

        channel = 0;
        for (int i = 0; i < numAudioOut; ++i)
        {
            String slug = "out_";
            slug << (i + 1);
            String name = "Out ";
            name << (i + 1);
            ports.add (PortType::Audio, index++, channel++, slug, name, false);
        }

        channel = 0;
        for (int i = 0; i < numMidiIn; ++i)
        {
            String slug = "midi_in_";
            slug << (i + 1);
            String name = "MIDI In ";
            name << (i + 1);
            ports.add (PortType::Midi, index++, channel++, slug, name, true);
        }

        channel = 0;
        for (int i = 0; i < numMidiOut; ++i)
        {
            String slug = "midi_out_";
            slug << (i + 1);
            String name = "MIDI Out ";
            name << (i + 1);
            ports.add (PortType::Midi, index++, channel++, slug, name, false);
        }
    } catch (const std::exception&)
    {
    }
}

element::ParameterPtr DSPScript::getParameterObject (int index, bool input) const
{
    return input ? inParams[index] : outParams[index];
}

void DSPScript::addParameter (const sol::table& param, bool input)
{
    const int channel = ports.size (PortType::Control, input);

    String name = param["name"].get_or (std::string ("Param ") + String (channel + 1).toStdString());
    String sym = param["symbol"].get_or (std::string());
    if (sym.isEmpty())
    {
        sym = name.trim().toLowerCase().replaceCharacter ('-', '_').replaceCharacter (' ', '_');
    }

    String type = param["type"].get_or (std::string ("float"));
    String flow = param["flow"].get_or (std::string ("input"));
    jassert (flow == "input" || flow == "output");

    float min = param["min"].get_or (0.0);
    float max = param["max"].get_or (1.0);
    float dfault = param["default"].get_or (1.0);
    ignoreUnused (min, max, dfault);

    // EL_LUA_DBG("index = " << index);
    // EL_LUA_DBG("channel = " << channel);
    // EL_LUA_DBG("is input = " << (int) input);
    // EL_LUA_DBG("name = " << name);
    // EL_LUA_DBG("symbol = " << sym);
    // EL_LUA_DBG("min = " << min);
    // EL_LUA_DBG("max = " << max);
    // EL_LUA_DBG("default = " << dfault);

    if (input)
    {
        paramData[channel] = dfault;
    }
    else
    {
        controlData[channel] = dfault;
    }

    ports.addControl (ports.size(),
                      channel,
                      sym,
                      name,
                      min,
                      max,
                      dfault,
                      input);
}

void DSPScript::addParameterPorts()
{
    sol::function f = DSP["layout"];
    if (! f.valid())
        return;

    try
    {
        sol::table layout = f();
        sol::table control = layout["control"].get_or_create<sol::table>();
        sol::table params = control[1].get_or_create<sol::table>();
        for (size_t i = 0; i < params.size(); ++i)
        {
            auto param = params[i + 1];
            addParameter (param, true);
        }

        sol::table controls = control[2].get_or_create<sol::table>();
        for (size_t i = 0; i < controls.size(); ++i)
        {
            auto param = controls[i + 1];
            addParameter (param, false);
        }

        numParams = ports.size (PortType::Control, true);
        numControls = ports.size (PortType::Control, false);

        unlinkParams();
        for (const auto* port : ports.getPorts())
        {
            if (port->type == PortType::Control && port->input)
                inParams.add (new Parameter (this, *port));
            else if (port->type == PortType::Control && ! port->input)
                outParams.add (new Parameter (this, *port));
        }
    } catch (const std::exception&)
    {
    }
}

void DSPScript::unlinkParams()
{
    for (auto* p : inParams)
        p->unlink();
    for (auto* p : outParams)
        p->unlink();

    inParams.clearQuick();
    outParams.clearQuick();
}

} // namespace element

using element::DSPScriptPosition;

EL_PLUGIN_EXPORT
int luaopen_el_DSPScriptPosition (lua_State* L)
{
    return DSPScriptPosition::loadModule (L);
}

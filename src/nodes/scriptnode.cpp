// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <math.h>

#include <element/midipipe.hpp>
#include <element/parameter.hpp>

#include "ElementApp.h"

#include "amp.lua.h"
#include "ampui.lua.h"
#include "channelize.lua.h"

#include "sol/sol.hpp"
#include "element/element.h"
#include "el/factories.hpp"
#include "engine/graphnode.hpp"
#include "nodes/scriptnode.hpp"
#include "scripting/bindings.hpp"
#include "scripting/dspscript.hpp"
#include "scripting/scriptloader.hpp"
#include "scripting/scriptmanager.hpp"

#define EL_LUA_DBG(x)
// #define EL_LUA_DBG(x) DBG(x)

static const juce::String initScript =
    R"(
require ('el.AudioBuffer')
require ('el.MidiBuffer')
require ('el.MidiMessage')
require ('el.midi')
require ('el.audio')
require ('el.MidiPipe')
)";

namespace element {

//=============================================================================
ScriptNode::ScriptNode() noexcept
    : Processor (0)
{
    setName ("Script");
    Lua::initializeState (lua);

    lua.set_function ("print", [this] (sol::variadic_args va) {
        auto& e = lua;
        String msg;
        for (auto v : va)
        {
            if (sol::type::string == v.get_type())
            {
                msg << v.as<const char*>() << " ";
                continue;
            }

            sol::function ts = e["tostring"];
            if (ts.valid())
            {
                sol::object str = ts ((sol::object) v);
                if (str.valid())
                    if (const char* sstr = str.as<const char*>())
                        msg << sstr << "  ";
            }
        }

        if (msg.isNotEmpty())
        {
            if (MessageManager::getInstance()->isThisTheMessageThread())
            {
                Logger::writeToLog (msg);
            }
            else
            {
                MessageManagerLock ml;
                Logger::writeToLog (msg);
            }
        }
    });

    script.reset (new DSPScript (lua.create_table()));
    dspCode.replaceAllContent (String::fromUTF8 (
        scripts::amp_lua, scripts::amp_luaSize));
    loadScript (dspCode.getAllContent());
    edCode.replaceAllContent (String::fromUTF8 (
        scripts::ampui_lua, scripts::ampui_luaSize));
    refreshPorts();
}

ScriptNode::~ScriptNode()
{
    script.reset();
}

void ScriptNode::refreshPorts()
{
    if (script == nullptr)
        return;
    PortList newPorts;
    script->getPorts (newPorts);
    setPorts (newPorts);
    if (auto g = getParentGraph())
        g->triggerAsyncUpdate();
}

void ScriptNode::setPlayHead (juce::AudioPlayHead* playhead)
{
    Processor::setPlayHead (playhead);
    if (script)
        script->setPlayHead (playhead);
}

ParameterPtr ScriptNode::getParameter (const PortDescription& port)
{
    jassert (port.type == PortType::Control);
    return script ? script->getParameterObject (port.channel, port.input) : nullptr;
}

Result ScriptNode::loadScript (const String& newCode, bool setDspCode)
{
    if (setDspCode) {
      dspCode.replaceAllContent (newCode);
      edCode.replaceAllContent ("");
    }

    auto result = DSPScript::validate (newCode);
    if (result.failed())
        return result;

    ScriptLoader loader (lua);
    loader.load (newCode);
    if (loader.hasError())
        return Result::fail (loader.getErrorMessage());

    auto dsp = loader();
    if (! dsp.valid() || dsp.get_type() != sol::type::table)
        return Result::fail ("Could not instantiate script");

    auto newScript = std::make_unique<DSPScript> (dsp);

    if (true)
    {
        newScript->setPlayHead (getPlayHead());
        if (prepared)
            newScript->prepare (sampleRate, blockSize);
        triggerPortReset();
        ScopedLock sl (lock);
        if (script != nullptr)
            newScript->copyParameterValues (*script);
        script.swap (newScript);
    }

    if (newScript != nullptr)
    {
        newScript->release();
        newScript->cleanup();
        newScript.reset();
    }

    return Result::ok();
}

void ScriptNode::getPluginDescription (PluginDescription& desc) const
{
    desc.name = "Script";
    desc.fileOrIdentifier = EL_NODE_ID_SCRIPT;
    desc.uniqueId = EL_NODE_UID_SCRIPT;
    desc.descriptiveName = "A user scriptable Element node";
    desc.numInputChannels = 0;
    desc.numOutputChannels = 0;
    desc.hasSharedContainer = false;
    desc.isInstrument = false;
    desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
    desc.pluginFormatName = EL_NODE_FORMAT_NAME;
    desc.version = "1.0.0";
}

void ScriptNode::prepareToRender (double rate, int block)
{
    if (prepared)
        return;
    sampleRate = rate;
    blockSize = block;
    script->prepare (sampleRate, blockSize);
    prepared = true;
}

void ScriptNode::releaseResources()
{
    if (! prepared)
        return;
    prepared = false;
    script->release();
}

void ScriptNode::render (RenderContext& rc)
{
    ScopedLock sl (lock);
    script->process (rc.audio, rc.midi);
}

void ScriptNode::setState (const void* data, int size)
{
    const auto state = ValueTree::readFromGZIPData (data, size);
    if (state.isValid())
    {
        dspCode.replaceAllContent (state["dspCode"].toString());
        edCode.replaceAllContent (state["editorCode"].toString());

        auto result = loadScript (dspCode.getAllContent());

        if (result.wasOk())
        {
            if (state.hasProperty ("data"))
            {
                const var& data = state.getProperty ("data");
                if (data.isBinaryData())
                    if (auto* block = data.getBinaryData())
                        script->restore (block->getData(), block->getSize());
            }
        }

        sendChangeMessage();
    }
}

void ScriptNode::getState (MemoryBlock& out)
{
    ValueTree state ("ScriptNode");
    state.setProperty ("dspCode", dspCode.getAllContent(), nullptr)
        .setProperty ("editorCode", edCode.getAllContent(), nullptr);

    MemoryBlock block;
    script->save (block);
    if (block.getSize() > 0)
        state.setProperty ("data", block, nullptr);
    block.reset();

    MemoryOutputStream mo (out, false);
    {
        GZIPCompressorOutputStream gz (mo);
        state.writeToStream (gz);
    }
}

void ScriptNode::setParameter (int index, float value)
{
    ScopedLock sl (lock);
}

//==============================================================================
const String ScriptNode::getProgramName (int index) const
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms()))
        return {};

    switch (index)
    {
        case 0:
            return "Amp";
            break;
        case 1:
            return "Channelizer";
            break;
        case 2:
            return "Load script...";
            break;
        case -1:
            return userScriptName;
            break;
    }

    String name = TRANS ("Program");
    name << " " << int (index + 1);
    return name;
}

void ScriptNode::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms()))
        return;
    _program = index;

    String newDspCode, newUiCode;

    switch (index)
    {
        case 0:
            newDspCode = String::fromUTF8 (scripts::amp_lua, scripts::amp_luaSize);
            newUiCode = String::fromUTF8 (scripts::ampui_lua, scripts::ampui_luaSize);
            break;
        case 1:
            newDspCode = String::fromUTF8 (scripts::channelize_lua, scripts::channelize_luaSize);
            newUiCode.clear();
            break;
        case 2:
            {
                FileChooser chooser ("Load Script", ScriptManager::getUserScriptsDir(), "*.lua");
                if (chooser.browseForFileToOpen()) {
                    juce::File dspFile(chooser.getResult());
                    newDspCode = dspFile.loadFileAsString();
                    newUiCode.clear();
                    userScriptName = dspFile.getFileName();
                    _program = -1;
                }
            }
            break;
    }

    dspCode.replaceAllContent (newDspCode);
    Result result = loadScript (dspCode.getAllContent());
    if (result.failed()) {
        userScriptName = "(invalid)";
    }
    edCode.replaceAllContent (newUiCode);
}

} // namespace element

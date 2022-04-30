/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#include "controllers/EngineController.h"
#include "controllers/GuiController.h"
#include "controllers/SessionController.h"
#include "controllers/MappingController.h"
#include "controllers/DevicesController.h"
#include "engine/InternalFormat.h"
#include "session/PluginManager.h"
#include "session/Session.h"
#include "ElementApp.h"
#include "Settings.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"

// #define PLUGIN_DBG(msg) DBG(msg)
#define PLUGIN_DBG(msg)

namespace Element {

//=============================================================================
static void setPluginMissingNodeProperties (const ValueTree& tree)
{
    if (tree.hasType (Tags::node))
    {
        const Node node (tree, true);
        ignoreUnused (node);
    }
    else if (tree.hasType (Tags::controller) || tree.hasType (Tags::control))
    {
        PLUGIN_DBG ("[EL] set missing for: " << tree.getProperty (Tags::name).toString());
    }
}

//=============================================================================
#define enginectl controller->findChild<EngineController>()
#define guictl controller->findChild<GuiController>()
#define sessionctl controller->findChild<SessionController>()
#define mapsctl controller->findChild<MappingController>()
#define devsctl controller->findChild<DevicesController>()

//=============================================================================
PluginProcessor::PluginProcessor (Variant instanceType, int numBuses)
    : AudioProcessor (createDefaultBuses (instanceType, jmax (0, numBuses))),
      variant (instanceType)
{
    setRateAndBufferSizeDetails (sampleRate, bufferSize);
    numIns = getTotalNumInputChannels();
    numOuts = getTotalNumOutputChannels();

    for (int i = 0; i < 8; ++i)
    {
        auto* param = new PerformanceParameter (i);
        addParameter (param);
        perfparams.add (param);
    }

    prepared = controllerActive = false;
    world.reset (new Globals());
    world->setEngine (new AudioEngine (*world, RunMode::Plugin));
    engine = world->getAudioEngine();
    SessionPtr session = world->getSession();
    Settings& settings (world->getSettings());
    PluginManager& plugins (world->getPluginManager());
    engine->applySettings (settings);

    plugins.addDefaultFormats();
    plugins.addFormat (new InternalFormat (*engine, world->getMidiEngine()));
    plugins.addFormat (new ElementAudioPluginFormat (*world));
    plugins.restoreUserPlugins (settings);

    // The hosts WILL release and prepare the plugin frequently at any given
    // time. plugins are handled different by each one, so it's best to keep
    // our engine running at all times to reduce massive plugin unloads and
    // re-loads back to back.
    engine->prepareExternalPlayback (sampleRate, bufferSize, getTotalNumInputChannels(), getTotalNumOutputChannels());
    session->clear();
    if (MessageManager::getInstance()->isThisTheMessageThread())
    {
        session->addGraph (Node::createDefaultGraph ("Graph 1"), true);
        PLUGIN_DBG ("[EL] default graph created");
    }
    else
    {
        PLUGIN_DBG ("[EL] couldn't create default graph");
    }

    controller.reset (new AppController (*world, RunMode::Plugin));
    controller->activate();
    controllerActive = true;

    enginectl->sessionReloaded();
    mapsctl->learn (false);
    devsctl->refresh();
    shouldProcess.set (true);

    asyncPrepare.reset (new AsyncPrepare (*this));
}

PluginProcessor::~PluginProcessor()
{
    asyncPrepare.reset();

    for (auto* param : perfparams)
        param->clearNode();
    perfparams.clear();

    if (controllerActive)
        controller->deactivate();

    engine->releaseExternalResources();

    if (auto session = world->getSession())
        session->clear();

    world->setEngine (nullptr);
    engine = nullptr;
    controller = nullptr;
    world = nullptr;
}

const String PluginProcessor::getName() const
{
    switch (variant)
    {
        case Instrument:
            return "Element";
            break;
        case Effect:
            return "Element FX";
            break;
        case MidiEffect:
            return "Element MFX";
            break;
        default:
            break;
    }

    jassertfalse;
    return "Element";
}

bool PluginProcessor::acceptsMidi() const { return true; }
bool PluginProcessor::producesMidi() const { return true; }
bool PluginProcessor::isMidiEffect() const { return variant == MidiEffect; }

double PluginProcessor::getTailLengthSeconds() const { return 0.0; }

int PluginProcessor::getNumPrograms() { return 1; }
int PluginProcessor::getCurrentProgram() { return 0; }
void PluginProcessor::setCurrentProgram (int index) { ignoreUnused (index); }
const String PluginProcessor::getProgramName (int index)
{
    ignoreUnused (index);
    return { "Default" };
}
void PluginProcessor::changeProgramName (int index, const String& newName) { ignoreUnused (index, newName); }

void PluginProcessor::prepareToPlay (double sr, int bs)
{
    if (! MessageManager::getInstance()->isThisTheMessageThread())
    {
        asyncPrepare->prepare (sr, bs);
        shouldProcess.set (false);
        return;
    }

    PLUGIN_DBG ("[EL] prepare to play: prepared=" << (int) prepared << " sampleRate: " << sampleRate << " buff: " << bufferSize << " numIns: " << numIns << " numOuts: " << numOuts);

    const bool channelCountsChanged = numIns != getTotalNumInputChannels()
                                      || numOuts != getTotalNumOutputChannels();
    const bool detailsChanged = sampleRate != sr || bufferSize != bs || channelCountsChanged;

    numIns = getTotalNumInputChannels();
    numOuts = getTotalNumOutputChannels();
    sampleRate = sr;
    bufferSize = bs;

    if (! prepared || detailsChanged)
    {
        prepared = true;

        auto& plugins (world->getPluginManager());
        plugins.setPlayConfig (sampleRate, bufferSize);

        if (detailsChanged)
        {
            PLUGIN_DBG ("[EL] details changed: " << sampleRate << " : " << bufferSize << " : " << getTotalNumInputChannels() << "/" << getTotalNumOutputChannels());

            if (channelCountsChanged) // && preparedCount <= 0)
            {
                engine->releaseExternalResources();
                engine->prepareExternalPlayback (sampleRate, bufferSize, getTotalNumInputChannels(), getTotalNumOutputChannels());
                updateLatencySamples();
            }

            triggerAsyncUpdate();
            preparedCount++;
        }
    }

    updateLatencySamples();
    engine->sampleLatencyChanged.connect (
        std::bind (&PluginProcessor::updateLatencySamples, this));

    shouldProcess.set (true);
}

void PluginProcessor::releaseResources()
{
    PLUGIN_DBG ("[EL] release resources: " << (int) prepared);
    if (engine)
        engine->sampleLatencyChanged.disconnect_all_slots();
    if (prepared)
    {
        prepared = false;
    }
}

void PluginProcessor::reloadEngine()
{
    jassert (MessageManager::getInstance()->isThisTheMessageThread());

    const bool wasSuspended = isSuspended();
    suspendProcessing (true);

    auto session = world->getSession();
    session->saveGraphState();
    engine->releaseExternalResources();
    engine->prepareExternalPlayback (sampleRate, bufferSize, getTotalNumInputChannels(), getTotalNumOutputChannels());
    updateLatencySamples();

    session->restoreGraphState();
    enginectl->sessionReloaded();
    enginectl->syncModels();
    guictl->stabilizeContent();
    devsctl->refresh();

    suspendProcessing (wasSuspended);
}

void PluginProcessor::reset()
{
    PLUGIN_DBG ("[EL] plugin reset");
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    bool supported = false;
    switch (variant)
    {
        case Instrument:
            // require at least a main output bus
            supported = layouts.outputBuses.size() > 0 && layouts.getNumChannels (false, 0) > 0;
            break;

        case Effect:
            // require at least a main input and output bus
            supported = layouts.inputBuses.size() > 0 && layouts.getNumChannels (true, 0) > 0 && layouts.outputBuses.size() > 0 && layouts.getNumChannels (false, 0) > 0;
            break;

        case MidiEffect:
            // buses not supported in midi effect mode
        default:
            supported = false;
            break;
    }

    return supported;
}

void PluginProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;

    if (! shouldProcess.get())
    {
        buffer.clear (0, buffer.getNumSamples());
        midi.clear();
        return;
    }

    if (auto* playhead = getPlayHead())
        if (engine->isUsingExternalClock())
            engine->processExternalPlayhead (playhead, buffer.getNumSamples());

    // clear garbage in extra output channels.
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    engine->processExternalBuffers (buffer, midi);
}

bool PluginProcessor::hasEditor() const { return true; }

AudioProcessorEditor* PluginProcessor::createEditor()
{
    if (auto* gui = controller->findChild<GuiController>())
        gui->stabilizeContent();
    return new PluginEditor (*this);
}

void PluginProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto session = world->getSession())
    {
        session->saveGraphState();
        session->getValueTree()
            .setProperty ("pluginEditorBounds", editorBounds.toString(), nullptr)
            .setProperty ("editorKeyboardFocus", editorWantsKeyboard, nullptr)
            .setProperty ("forceZeroLatency", isForcingZeroLatency(), nullptr);
        
        if (auto engine = world->getAudioEngine())
            if (auto mon = engine->getTransportMonitor())
                session->getValueTree().setProperty (
                    "pluginTransportPlaying", mon->playing.get(), nullptr);

        auto ppData = session->getValueTree().getOrCreateChildWithName ("perfParams", nullptr);
        ppData.removeAllChildren (nullptr);
        for (auto* const pp : perfparams)
        {
            if (! pp->haveNode())
                continue;
            ValueTree data ("perfParam");
            data.setProperty (Tags::index, pp->getParameterIndex(), nullptr)
                .setProperty (Tags::node, pp->getNode().getUuidString(), nullptr)
                .setProperty (Tags::parameter, pp->getBoundParameter(), nullptr);
            ppData.appendChild (data, nullptr);
        }

        if (auto xml = std::unique_ptr<XmlElement> (session->createXml()))
            copyXmlToBinary (*xml, destData);
    }
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    PLUGIN_DBG ("[EL] restore state: prepared: " << (int) prepared);

    auto session = world->getSession();
    auto engine = world->getAudioEngine();
    if (! session || ! shouldProcess.get())
        return;

    mapsctl->learn (false);

    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        String error;
        ValueTree newData = ValueTree::fromXml (*xml);
        if (! newData.isValid() || ! newData.hasType (Tags::session))
            error = "Invalid session state information provided.";
        if (error.isEmpty() && ! session->loadData (newData))
            error = "Could not load session data.";

        if (error.isNotEmpty())
        {
            PLUGIN_DBG ("[EL] plugin failed restoring state: " << error);
        }
        else
        {
            typedef Rectangle<int> RI;
            editorBounds = RI::fromString (session->getProperty (
                                                      "pluginEditorBounds", RI().toString())
                                               .toString());
            editorWantsKeyboard = (bool) session->getProperty ("editorKeyboardFocus", false);
            setForceZeroLatency ((bool)session->getProperty ("forceZeroLatency", isForcingZeroLatency()));
            if (engine && ! engine->isUsingExternalClock())
                engine->setPlaying ((bool) session->getProperty ("pluginTransportPlaying", false));
            session->forEach (setPluginMissingNodeProperties);
            for (auto* const param : perfparams)
                param->clearNode();
        }

        triggerAsyncUpdate();

        if (prepared)
        {
            PLUGIN_DBG ("[EL] plugin restored state while already prepared");
        }
        else
        {
            PLUGIN_DBG ("[EL] plugin tried to restore state when not prepared");
        }
    }
}

void PluginProcessor::numChannelsChanged()
{
    PLUGIN_DBG ("[EL] num channels changed: " << getTotalNumInputChannels() << "/" << getTotalNumOutputChannels());
}

void PluginProcessor::numBusesChanged()
{
    PLUGIN_DBG ("[EL] num buses changed: " << getBusCount (true) << "/" << getBusCount (false));
}

void PluginProcessor::processorLayoutsChanged()
{
    PLUGIN_DBG ("[EL] layout changed: prepared: " << (int) prepared);
    triggerAsyncUpdate();
}

void PluginProcessor::handleAsyncUpdate()
{
    PLUGIN_DBG ("[EL] handle async update");
    reloadEngine();

    auto session = world->getSession();
    const auto ppData = session->getValueTree().getChildWithName ("perfParams");

    for (int i = 0; i < ppData.getNumChildren(); ++i)
    {
        const auto data = ppData.getChild (i);
        const int index = (int) data[Tags::index];
        if (! isPositiveAndBelow (index, 8))
            continue;
        const int parameter = (int) data[Tags::parameter];
        const String uuid = data[Tags::node].toString();
        if (uuid.isEmpty())
            continue;
        const Node node = session->findNodeById (Uuid (uuid));
        auto* const param = perfparams[index];
        if (param != nullptr && node.isValid())
            param->bindToNode (node, parameter);
    }

    onPerfParamsChanged();
}

bool PluginProcessor::isNodeBoundToAnyPerformanceParameter (const Node& boundNode, int boundParam) const
{
    if (! boundNode.isValid() || boundParam == NodeObject::NoParameter)
        return false;
    for (auto* const pp : perfparams)
        if (boundNode == pp->getNode() && boundParam == pp->getBoundParameter())
            return true;
    return false;
}

PopupMenu PluginProcessor::getPerformanceParameterMenu (int perfParam)
{
    auto* const paramObj = perfparams[perfParam];
    if (nullptr == paramObj)
        return PopupMenu();

    auto session = world->getSession();
    PopupMenu menu;
    int menuIdx = 0;
    menuMap.clearQuick (true);

    for (int i = 0; i < session->getNumGraphs(); ++i)
    {
        auto graph = session->getGraph (i);
        for (int j = 0; j < graph.getNumNodes(); ++j)
        {
            PopupMenu submenu;
            auto node = graph.getNode (j);
            NodeObjectPtr ptr = node.getObject();
            if (ptr == nullptr)
                continue;
            auto* proc = ptr->getAudioProcessor();
            if (proc == nullptr)
                continue;

            for (int k = 0; k < proc->getParameters().size(); ++k)
            {
                auto* const param = proc->getParameters()[k];
                if (! param->isAutomatable())
                    continue;

                const bool isMine = paramObj->getNode() == node && k == paramObj->getBoundParameter();
                const bool isBound = isNodeBoundToAnyPerformanceParameter (node, k);
                submenu.addItem (++menuIdx, param->getName (100), ! isBound || isMine, isMine);

                auto* const item = menuMap.add (new PerfParamMenuItem());
                item->node = node;
                item->parameter = k;
            }

            if (submenu.getNumItems() > 0)
                menu.addSubMenu (node.getName(), submenu);
        }
    }

    if (menu.getNumItems() > 0 && isNodeBoundToAnyPerformanceParameter (paramObj->getNode(), paramObj->getBoundParameter()))
    {
        menu.addSeparator();
        menu.addItem (++menuIdx, "Unlink");
        auto* const item = menuMap.add (new PerfParamMenuItem());
        item->node = paramObj->getNode();
        item->parameter = paramObj->getBoundParameter();
        item->unlink = true;
    }

    return menu;
}

void PluginProcessor::handlePerformanceParameterResult (int result, int perfParam)
{
    auto* const param = perfparams[perfParam];
    if (! param)
        return;

    if (auto* item = menuMap[result - 1])
    {
        if (item->unlink)
        {
            param->clearNode();
        }
        else
        {
            const bool wasAlreadyBound = param->haveNode() && param->getNode() == item->node && param->getBoundParameter() == item->parameter;
            param->clearNode();
            if (! wasAlreadyBound)
                param->bindToNode (item->node, item->parameter);
        }

        param->updateValue();
    }

    onPerfParamsChanged();
    menuMap.clearQuick (true);
}

void PluginProcessor::setForceZeroLatency (bool force)
{
    if (force == forceZeroLatency)
        return;
    forceZeroLatency = force;
    updateLatencySamples();
}

int PluginProcessor::calculateLatencySamples() const
{
    if (forceZeroLatency)
        return 0;
    return engine != nullptr ? engine->getExternalLatencySamples()
                             : 0;
}

void PluginProcessor::updateLatencySamples()
{
    setLatencySamples (calculateLatencySamples());
}

//=============================================================================
AudioProcessor::BusesProperties PluginProcessor::createDefaultBuses (PluginProcessor::Variant variant, int numAux)
{
    if (variant == PluginProcessor::MidiEffect)
        return {};

    const auto stereo = AudioChannelSet::stereo();

    AudioProcessor::BusesProperties buses;
    buses.addBus (true, "Main", stereo, variant == PluginProcessor::Effect);
    buses.addBus (false, "Main", stereo, true);
    for (int i = 0; i < numAux; ++i)
    {
        String name = "Aux ";
        name << String (i + 1);
        buses.addBus (true, name, stereo, false);
        buses.addBus (false, name, stereo, false);
    }

    return buses;
}

} // namespace Element

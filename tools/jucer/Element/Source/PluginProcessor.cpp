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

#include "Settings.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "../../../libs/compat/BinaryData.cpp"

//=============================================================================
static void setPluginMissingNodeProperties (const ValueTree& tree)
{
    if (tree.hasType (Tags::node))
    {
        const Node node (tree, true);
        ignoreUnused (node);
    }
    else if (tree.hasType (Tags::controller) ||
             tree.hasType (Tags::control))
    {
        DBG("[EL] set missing for: " << tree.getProperty(Tags::name).toString());
    }
}

//=============================================================================
#define enginectl controller->findChild<EngineController>()
#define guictl controller->findChild<GuiController>()
#define sessionctl controller->findChild<SessionController>()
#define mapsctl controller->findChild<MappingController>()
#define devsctl controller->findChild<DevicesController>()

//=============================================================================
ElementPluginAudioProcessor::ElementPluginAudioProcessor()
    : AudioProcessor (BusesProperties()
       #if JucePlugin_IsSynth
        .withInput  ("Main",  AudioChannelSet::stereo(), false)
        .withInput  ("Aux 1", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 2", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 3", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 4", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 5", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 6", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 7", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 8", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 9", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 10", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 11", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 12", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 13", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 14", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 15", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 16", AudioChannelSet::stereo(), false)
        
        .withOutput ("Main",  AudioChannelSet::stereo(), true)
        .withOutput  ("Aux 1", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 2", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 3", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 4", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 5", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 6", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 7", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 8", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 9", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 10", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 11", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 12", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 13", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 14", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 15", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 16", AudioChannelSet::stereo(), false))
       #else
        .withInput  ("Main",  AudioChannelSet::stereo(), true)
        .withInput  ("Aux 1", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 2", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 3", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 4", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 5", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 6", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 7", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 8", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 9", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 10", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 11", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 12", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 13", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 14", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 15", AudioChannelSet::stereo(), false)
        .withInput  ("Aux 16", AudioChannelSet::stereo(), false)
        
        .withOutput ("Main",  AudioChannelSet::stereo(), true)
        .withOutput  ("Aux 1", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 2", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 3", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 4", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 5", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 6", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 7", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 8", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 9", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 10", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 11", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 12", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 13", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 14", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 15", AudioChannelSet::stereo(), false)
        .withOutput  ("Aux 16", AudioChannelSet::stereo(), false))
       #endif
{
    for (int i = 0; i < 8; ++i)
    {
        auto* param = new PerformanceParameter (i);
        addParameter (param);
        perfparams.add (param);
    }

    prepared = controllerActive = false;
    world = new Globals();
    
    controller = new AppController (*world);
    engine = new AudioEngine (*world);
    world->setEngine (engine);
    SessionPtr session = world->getSession();

    if (true)
    {
        Settings& settings (world->getSettings());
        PluginManager& plugins (world->getPluginManager());
        engine->applySettings (settings);

        plugins.addDefaultFormats();
        plugins.addFormat (new InternalFormat (*engine, world->getMidiEngine()));
        plugins.addFormat (new ElementAudioPluginFormat (*world));
        plugins.restoreUserPlugins (settings);

        // The hosts WILL release and prepare the plugin frequently at any given time.
        // plugins are handled different by each one, so it's best to keep our engine
        // running at all times to reduce massive plugin unloads and re-loads back to back.
        engine->prepareExternalPlayback (sampleRate, bufferSize,
                                         getTotalNumInputChannels(),
                                         getTotalNumOutputChannels());
        session->clear();
        if (MessageManager::getInstance()->isThisTheMessageThread())
        {
            session->addGraph (Node::createDefaultGraph ("Graph 1"), true);
            DBG("[EL] default graph created");
        }
        else
        {
            DBG("[EL] couldn't create default graph");
        }
        controller->activate();
        controllerActive = true;

        enginectl->sessionReloaded();
        mapsctl->learn (false);
        devsctl->refresh();
        shouldProcess.set (true);
    }

    asyncPrepare.reset (new AsyncPrepare (*this));
}

ElementPluginAudioProcessor::~ElementPluginAudioProcessor()
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
    controller = nullptr;
    world = nullptr;
}

const String ElementPluginAudioProcessor::getName() const
{
    return "Element";
}

bool ElementPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ElementPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ElementPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ElementPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ElementPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int ElementPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ElementPluginAudioProcessor::setCurrentProgram (int index)
{
}

const String ElementPluginAudioProcessor::getProgramName (int index)
{
    return  { "Default" };
}

void ElementPluginAudioProcessor::changeProgramName (int index, const String& newName)
{
}

void ElementPluginAudioProcessor::prepareToPlay(double sr, int bs)
{
    if (!MessageManager::getInstance()->isThisTheMessageThread())
    {
        asyncPrepare->prepare (sr, bs);
        shouldProcess.set (false);
        return;
    }

    DBG("[EL] prepare to play: prepared=" << (int) prepared <<
        " sampleRate: " << sampleRate <<
        " buff: " << bufferSize <<
		" numIns: " << numIns <<
        " numOuts: " << numOuts);
    
    const bool channelCountsChanged = numIns != getTotalNumInputChannels()
                                   || numOuts != getTotalNumOutputChannels();
    const bool detailsChanged = sampleRate != sr || bufferSize != bs || channelCountsChanged;
    
	numIns		= getTotalNumInputChannels();
	numOuts		= getTotalNumOutputChannels();
	sampleRate	= sr;
	bufferSize	= bs;

    if (! prepared || detailsChanged)
    {
        prepared = true;

        auto& plugins (world->getPluginManager());
        plugins.setPlayConfig (sampleRate, bufferSize);
        
        if (detailsChanged)
        {
            DBG("[EL] details changed: " << sampleRate << " : " << bufferSize << " : " <<
                 getTotalNumInputChannels() << "/" << getTotalNumOutputChannels());
            
            if (channelCountsChanged) // && preparedCount <= 0)
            {
                engine->releaseExternalResources();
                engine->prepareExternalPlayback (sampleRate, bufferSize,
                                                 getTotalNumInputChannels(),
                                                 getTotalNumOutputChannels());
                updateLatencySamples();
            }
            
            triggerAsyncUpdate();
            preparedCount++;
        }
    }

    updateLatencySamples();
    engine->sampleLatencyChanged.connect (
        std::bind (&ElementPluginAudioProcessor::updateLatencySamples, this));

    shouldProcess.set (true);
}

void ElementPluginAudioProcessor::releaseResources()
{
    DBG("[EL] release resources: " << (int) prepared);
    if (engine)
        engine->sampleLatencyChanged.disconnect_all_slots();
    if (prepared)
    {
        prepared = false;
    }
}

void ElementPluginAudioProcessor::reloadEngine()
{
    jassert(MessageManager::getInstance()->isThisTheMessageThread());

    const bool wasSuspended = isSuspended();
    suspendProcessing (true);

    auto session = world->getSession();
    session->saveGraphState();
    engine->releaseExternalResources();
    engine->prepareExternalPlayback (sampleRate, bufferSize,
                                     getTotalNumInputChannels(),
                                     getTotalNumOutputChannels());
    updateLatencySamples();

    session->restoreGraphState();
    enginectl->sessionReloaded();
    enginectl->syncModels();
    guictl->stabilizeContent();
    devsctl->refresh();
    
    suspendProcessing (wasSuspended);
}

void ElementPluginAudioProcessor::reset()
{
    DBG("[EL] plugin reset");
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ElementPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;

  #else
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
          && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ElementPluginAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midi)
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

bool ElementPluginAudioProcessor::hasEditor() const { return true; }

AudioProcessorEditor* ElementPluginAudioProcessor::createEditor()
{
    if (auto* gui = controller->findChild<GuiController>())
        gui->stabilizeContent();
    return new ElementPluginAudioProcessorEditor (*this);
}

void ElementPluginAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto session = world->getSession())
    {
        session->saveGraphState();
        session->getValueTree()
            .setProperty ("pluginEditorBounds", editorBounds.toString(), nullptr)
            .setProperty ("editorKeyboardFocus", editorWantsKeyboard, nullptr)
            .setProperty ("forceZeroLatency", isForcingZeroLatency(), nullptr);
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

void ElementPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    DBG("[EL] restore state: prepared: " << (int) prepared);
    
    auto session = world->getSession();
    if (! session || ! shouldProcess.get())
        return;
    
    mapsctl->learn (false);
    
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        String error;
        ValueTree newData = ValueTree::fromXml (*xml);
        if (!newData.isValid() || !newData.hasType (Tags::session))
            error = "Invalid session state information provided.";
        if (error.isEmpty() && !session->loadData (newData))
            error = "Could not load session data.";
        
        if (error.isNotEmpty())
        {
            DBG("[EL] plugin failed restoring state: " << error);
        }
        else
        {
            typedef Rectangle<int> RI;
            editorBounds = RI::fromString (session->getProperty (
                "pluginEditorBounds", RI().toString()).toString());
            editorWantsKeyboard = (bool) session->getProperty ("editorKeyboardFocus", false);
            setForceZeroLatency ((bool)session->getProperty ("forceZeroLatency", isForcingZeroLatency()));
            session->forEach (setPluginMissingNodeProperties);
            for (auto* const param : perfparams)
                param->clearNode();
        }
        
        triggerAsyncUpdate();
        
        if (prepared)
        {
            DBG("[EL] plugin restored state while already prepared");
        }
        else
        {
            DBG("[EL] plugin tried to restore state when not prepared");
        }
    }
}

void ElementPluginAudioProcessor::numChannelsChanged()
{
    DBG("[EL] num channels changed: " <<
        getTotalNumInputChannels() << "/" << getTotalNumOutputChannels());
}

void ElementPluginAudioProcessor::numBusesChanged()
{
    DBG("[EL] num buses changed: " <<
        getBusCount (true) << "/" << getBusCount (false));
}

void ElementPluginAudioProcessor::processorLayoutsChanged()
{
    DBG("[EL] layout changed: prepared: " << (int) prepared);
    triggerAsyncUpdate();
}

void ElementPluginAudioProcessor::handleAsyncUpdate()
{
    DBG("[EL] handle async update");
    reloadEngine();
    
    auto session = world->getSession();
    const auto ppData = session->getValueTree().getChildWithName ("perfParams");
    
    for (int i = 0; i < ppData.getNumChildren(); ++i)
    {
        const auto data = ppData.getChild (i);
        const int index         = (int) data [Tags::index];
        if (! isPositiveAndBelow (index, 8))
            continue;
        const int parameter     = (int) data [Tags::parameter];
        const String uuid       = data[Tags::node].toString();
        if (uuid.isEmpty())
            continue;
        const Node node         = session->findNodeById (Uuid (uuid));
        auto* const param       = perfparams [index];
        if (param != nullptr && node.isValid())
            param->bindToNode (node, parameter);
    }

    onPerfParamsChanged();
}

bool ElementPluginAudioProcessor::isNodeBoundToAnyPerformanceParameter (const Node& boundNode, int boundParam) const
{
    if (! boundNode.isValid() || boundParam == GraphNode::NoParameter)
        return false;
    for (auto* const pp : perfparams)
        if (boundNode == pp->getNode() && boundParam == pp->getBoundParameter())
            return true;
    return false;
}

PopupMenu ElementPluginAudioProcessor::getPerformanceParameterMenu (int perfParam)
{ 
    auto* const paramObj = perfparams [perfParam];
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
            GraphNodePtr ptr = node.getGraphNode();
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
                
                const bool isMine  = paramObj->getNode() == node && k == paramObj->getBoundParameter();
                const bool isBound = isNodeBoundToAnyPerformanceParameter (node, k);
                submenu.addItem (++menuIdx, param->getName (100), !isBound || isMine, isMine);

                auto* const item = menuMap.add (new PerfParamMenuItem ());
                item->node = node;
                item->parameter = k;
            }

            if (submenu.getNumItems() > 0)
                menu.addSubMenu (node.getName(), submenu);
        }
    }

    if (menu.getNumItems() > 0 &&
        isNodeBoundToAnyPerformanceParameter (paramObj->getNode(), paramObj->getBoundParameter()))
    {
        menu.addSeparator();
        menu.addItem (++menuIdx, "Unlink");
        auto* const item = menuMap.add (new PerfParamMenuItem ());
        item->node = paramObj->getNode();
        item->parameter = paramObj->getBoundParameter();
        item->unlink = true;
    }

    return menu;
}

void ElementPluginAudioProcessor::handlePerformanceParameterResult (int result, int perfParam)
{
    auto* const param = perfparams [perfParam];
    if (! param)
        return;
    
    if (auto* item = menuMap [result - 1])
    {
        if (item->unlink)
        {
            param->clearNode();
        }
        else
        {
            const bool wasAlreadyBound = param->haveNode() && 
                param->getNode() == item->node && 
                param->getBoundParameter() == item->parameter;
            param->clearNode();
            if (! wasAlreadyBound)
                param->bindToNode (item->node, item->parameter);
        }

        param->updateValue();
    }
    
    onPerfParamsChanged();
    menuMap.clearQuick (true);
}

void ElementPluginAudioProcessor::setForceZeroLatency (bool force)
{
    if (force == forceZeroLatency)
        return;
    forceZeroLatency = force;
    updateLatencySamples();
}

int ElementPluginAudioProcessor::calculateLatencySamples() const
{
    if (forceZeroLatency)
        return 0;
    return engine != nullptr ? engine->getExternalLatencySamples()
                             : 0;
}

void ElementPluginAudioProcessor::updateLatencySamples()
{
    setLatencySamples (calculateLatencySamples());
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ElementPluginAudioProcessor();
}

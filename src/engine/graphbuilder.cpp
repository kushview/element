// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/atombuffer.hpp>
#include <element/symbolmap.hpp>
#include <element/processor.hpp>

#include "engine/miditranspose.hpp"
#include "engine/graphnode.hpp"
#include "engine/graphbuilder.hpp"
#include "engine/ionode.hpp"

#ifndef EL_TRACE_GRAPH_OPS
#define EL_TRACE_GRAPH_OPS 0
#endif

namespace element {

using SharedMidi = OwnedArray<MidiBuffer>;
using SharedAtom = OwnedArray<AtomBuffer>;
using SharedAudio = AudioSampleBuffer;

class ApplyParamToCVOp : public GraphOp
{
public:
    ApplyParamToCVOp (ParameterPtr src, int _cvIndex)
        : param (src), cvIndex (std::max (0, _cvIndex))
    {
        value.setCurrentAndTargetValue (param->getValue());
    }

    void perform (AudioSampleBuffer& buffer, const OwnedArray<MidiBuffer>&, const SharedAtom&, const int nframes) override
    {
        value.setTargetValue (param->getValue());
        auto ptr = buffer.getWritePointer (cvIndex);
        for (int f = 0; f < nframes; ++f)
            ptr[f] = value.getNextValue();
    }

private:
    ParameterPtr param;
    LinearSmoothedValue<float> value;
    int cvIndex = 0;
};

class BindParameterOp : public GraphOp,
                        public Parameter::Listener
{
public:
    BindParameterOp (ParameterPtr src, ParameterPtr dst)
        : param1 (src), param2 (dst)
    {
        param1->addListener (this);
    }

    ~BindParameterOp()
    {
        param1->removeListener (this);
    }

    void controlValueChanged (int index, float value) override
    {
        juce::ignoreUnused (index);
        param2->setValueNotifyingHost (value);
    }

    void controlTouched (int index, bool grabbed) override
    {
    }

    void perform (AudioSampleBuffer&, const SharedMidi&, const SharedAtom&, const int) override {}

private:
    ParameterPtr param1, param2;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BindParameterOp)
};

//==============================================================================
class CopyAtomBufferOp : public GraphOp
{
public:
    CopyAtomBufferOp (const int srcBufferNum_, const int dstBufferNum_)
        : srcBufferNum (srcBufferNum_),
          dstBufferNum (dstBufferNum_)
    {
    }

    std::string traceStep() const noexcept
    {
        String str;
        str << "CopyAtomBuffer: buffers " << srcBufferNum << " to " << dstBufferNum;
        return str.toStdString();
    }

    void perform (AudioSampleBuffer&, const OwnedArray<MidiBuffer>&, const SharedAtom& atom, const int)
    {
        auto dst = atom.getUnchecked (dstBufferNum);
        dst->clear();
        dst->add (*atom.getUnchecked (srcBufferNum));
    }

private:
    const int srcBufferNum, dstBufferNum;

    JUCE_DECLARE_NON_COPYABLE (CopyAtomBufferOp)
};

class AddAtomBufferOp : public GraphOp
{
public:
    AddAtomBufferOp (const int srcBufferNum_, const int dstBufferNum_)
        : srcBufferNum (srcBufferNum_),
          dstBufferNum (dstBufferNum_)
    {
    }

    void perform (AudioSampleBuffer&, const OwnedArray<MidiBuffer>&, const SharedAtom& atom, const int numSamples)
    {
        atom.getUnchecked (dstBufferNum)
            ->add (*atom.getUnchecked (srcBufferNum)); // TODO: -> , 0, numSamples, 0);
    }

private:
    const int srcBufferNum, dstBufferNum;

    JUCE_DECLARE_NON_COPYABLE (AddAtomBufferOp)
};

class ClearAtomBufferOp : public GraphOp
{
    const int bufferIdx;

public:
    ClearAtomBufferOp (int b)
        : bufferIdx (b) {}

    std::string traceStep() const noexcept override
    {
        String str;
        str << "ClearAtomBufferOp: buffer " << bufferIdx;
        return str.toStdString();
    }

    void perform (SharedAudio&, const SharedMidi&, const SharedAtom& atom, const int numSamples) override
    {
        atom.getUnchecked (bufferIdx)->clear (0, numSamples);
    }
};

class MidiToAtomOp : public GraphOp
{
public:
    MidiToAtomOp (int midiIndex, int atomIndex)
        : _midiIdx (midiIndex), _atomIdx (atomIndex) {}

    std::string traceStep() const noexcept override
    {
        String str;
        str << "MidiToAtomOp: midi " << _midiIdx << " to atom " << _atomIdx;
        return str.toStdString();
    }

    void perform (SharedAudio&, const SharedMidi& midi, const SharedAtom& atom, const int) override
    {
        atom.getUnchecked (_atomIdx)->add (*midi.getUnchecked (_midiIdx));
    }

private:
    const int _midiIdx, _atomIdx;
};

class AtomToMidiOp : public GraphOp
{
public:
    AtomToMidiOp (int atomIndex, int midiIndex, uint32_t midiEventURID) noexcept
        : _atomIdx (atomIndex),
          _midiIdx (midiIndex),
          midi_MidiEvent (midiEventURID) {}

    std::string traceStep() const noexcept override
    {
        String str;
        str << "AtomToMidiOp: atom " << _atomIdx << " to midi " << _midiIdx;
        return str.toStdString();
    }

    void perform (SharedAudio&, const SharedMidi& midi, const SharedAtom& atom, const int nframes) override
    {
        auto seq = atom.getUnchecked (_atomIdx)->sequence();
        auto mb = midi.getUnchecked (_midiIdx);

        LV2_ATOM_SEQUENCE_FOREACH (seq, ev)
        {
            if (ev->time.frames > nframes)
                break;
            if (ev->body.type != midi_MidiEvent)
                continue;
            mb->addEvent (LV2_ATOM_BODY (&ev->body),
                          ev->body.size,
                          static_cast<int> (ev->time.frames));
        }
    }

private:
    const int _atomIdx, _midiIdx;
    const uint32_t midi_MidiEvent;
};

//==============================================================================
class ClearChannelOp : public GraphOp
{
public:
    ClearChannelOp (const int channelNum_)
        : channelNum (channelNum_)
    {
    }

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray<MidiBuffer>&, const SharedAtom&, const int numSamples)
    {
        sharedBufferChans.clear (channelNum, 0, numSamples);
    }

private:
    const int channelNum;

    JUCE_DECLARE_NON_COPYABLE (ClearChannelOp)
};

class CopyChannelOp : public GraphOp
{
public:
    CopyChannelOp (const int srcChannelNum_, const int dstChannelNum_)
        : srcChannelNum (srcChannelNum_),
          dstChannelNum (dstChannelNum_)
    {
    }

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray<MidiBuffer>&, const SharedAtom&, const int numSamples)
    {
        sharedBufferChans.copyFrom (dstChannelNum, 0, sharedBufferChans, srcChannelNum, 0, numSamples);
    }

private:
    const int srcChannelNum, dstChannelNum;

    JUCE_DECLARE_NON_COPYABLE (CopyChannelOp)
};

class AddChannelOp : public GraphOp
{
public:
    AddChannelOp (const int srcChannelNum_, const int dstChannelNum_)
        : srcChannelNum (srcChannelNum_),
          dstChannelNum (dstChannelNum_)
    {
    }

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray<MidiBuffer>&, const SharedAtom&, const int numSamples)
    {
        sharedBufferChans.addFrom (dstChannelNum, 0, sharedBufferChans, srcChannelNum, 0, numSamples);
    }

private:
    const int srcChannelNum, dstChannelNum;

    JUCE_DECLARE_NON_COPYABLE (AddChannelOp)
};

class ClearMidiBufferOp : public GraphOp
{
public:
    ClearMidiBufferOp (const int bufferNum_)
        : bufferNum (bufferNum_)
    {
    }

    void perform (AudioSampleBuffer&, const OwnedArray<MidiBuffer>& sharedMidiBuffers, const SharedAtom&, const int)
    {
        sharedMidiBuffers.getUnchecked (bufferNum)->clear();
    }

private:
    const int bufferNum;

    JUCE_DECLARE_NON_COPYABLE (ClearMidiBufferOp)
};

//==============================================================================
class CopyMidiBufferOp : public GraphOp
{
public:
    CopyMidiBufferOp (const int srcBufferNum_, const int dstBufferNum_)
        : srcBufferNum (srcBufferNum_),
          dstBufferNum (dstBufferNum_)
    {
    }

    void perform (AudioSampleBuffer&, const OwnedArray<MidiBuffer>& sharedMidiBuffers, const SharedAtom&, const int)
    {
        *sharedMidiBuffers.getUnchecked (dstBufferNum) = *sharedMidiBuffers.getUnchecked (srcBufferNum);
    }

private:
    const int srcBufferNum, dstBufferNum;

    JUCE_DECLARE_NON_COPYABLE (CopyMidiBufferOp)
};

class AddMidiBufferOp : public GraphOp
{
public:
    AddMidiBufferOp (const int srcBufferNum_, const int dstBufferNum_)
        : srcBufferNum (srcBufferNum_),
          dstBufferNum (dstBufferNum_)
    {
    }

    void perform (AudioSampleBuffer&, const OwnedArray<MidiBuffer>& sharedMidiBuffers, const SharedAtom&, const int numSamples)
    {
        sharedMidiBuffers.getUnchecked (dstBufferNum)
            ->addEvents (*sharedMidiBuffers.getUnchecked (srcBufferNum), 0, numSamples, 0);
    }

private:
    const int srcBufferNum, dstBufferNum;

    JUCE_DECLARE_NON_COPYABLE (AddMidiBufferOp)
};

class DelayChannelOp : public GraphOp
{
public:
    DelayChannelOp (const int channel_, const int numSamplesDelay_)
        : channel (channel_),
          bufferSize (numSamplesDelay_ + 1),
          readIndex (0),
          writeIndex (numSamplesDelay_)
    {
        buffer.calloc ((size_t) bufferSize);
    }

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray<MidiBuffer>&, const SharedAtom&, const int numSamples)
    {
        float* data = sharedBufferChans.getWritePointer (channel, 0);

        for (int i = numSamples; --i >= 0;)
        {
            buffer[writeIndex] = *data;
            *data++ = buffer[readIndex];

            if (++readIndex >= bufferSize)
                readIndex = 0;
            if (++writeIndex >= bufferSize)
                writeIndex = 0;
        }
    }

private:
    HeapBlock<float> buffer;
    const int channel, bufferSize;
    int readIndex, writeIndex;

    JUCE_DECLARE_NON_COPYABLE (DelayChannelOp)
};

class ProcessBufferOp : public GraphOp
{
public:
    ProcessBufferOp (const ProcessorPtr& node_,
                     const int totalChans_,
                     const int totalCV_,
                     const int midiBufferToUse_,
                     const Array<int> chans[PortType::Unknown])
        : node (node_),
          processor (node_->getAudioPluginInstance()),
          audioChannelsToUse (chans[PortType::Audio]),
          cvChannelsToUse (chans[PortType::CV]),
          midiChannelsToUse (chans[PortType::Midi]),
          atomChannelsToUse (chans[PortType::Atom]),
          totalChans (std::max (1, totalChans_)),
          totalCV (std::max (1, totalCV_)),
          numAudioIns (node_->getNumPorts (PortType::Audio, true)),
          numAudioOuts (node_->getNumPorts (PortType::Audio, false)),
          midiBufferToUse (midiBufferToUse_)
    {
        channels.calloc ((size_t) totalChans);
        cv.calloc ((size_t) totalCV);

        while (audioChannelsToUse.size() < totalChans)
            audioChannelsToUse.add (0);
        while (cvChannelsToUse.size() < totalCV)
            cvChannelsToUse.add (0);

        if (midiChannelsToUse.size() > 0)
            midiBufferToUse = midiChannelsToUse.getFirst();
        else
            midiChannelsToUse.add (midiBufferToUse);

        if (atomChannelsToUse.isEmpty())
            atomChannelsToUse.add (0);

        lastMute = node->isMuted();

        osChanSize = totalChans;
        osChans.reset (new float*[osChanSize]);
        tempMidi.ensureSize (128);
    }

    void perform (SharedAudio& sharedBufferChans,
                  const SharedMidi& sharedMidiBuffers,
                  const SharedAtom& sharedAtomBuffers,
                  const int numSamples) override
    {
        for (int i = totalChans; --i >= 0;)
            channels[i] = sharedBufferChans.getWritePointer (audioChannelsToUse.getUnchecked (i), 0);
        for (int i = totalCV; --i >= 0;)
            cv[i] = sharedBufferChans.getWritePointer (cvChannelsToUse.getUnchecked (i), 0);

        // clang-format off
        RenderContext context (channels, totalChans, cv, totalCV, 
                               sharedMidiBuffers, midiChannelsToUse, 
                               sharedAtomBuffers, atomChannelsToUse,
                               numSamples);
        // clang-format on

        if (! node->isEnabled())
        {
            for (int ch = numAudioIns; ch < numAudioOuts; ++ch)
                context.audio.clear (ch, 0, numSamples);
            return;
        }

        const bool muted = node->isMuted();
        const bool muteInput = node->isMutingInputs();

        if (muted && muteInput)
        {
            if (lastMute != muted)
            {
                // just became muted
                context.audio.applyGainRamp (0, numSamples, node->getLastInputGain(), 0.0);
            }
            else
            {
                // normal mute processing
                context.audio.applyGain (0, numSamples, 0.0);
            }
        }
        else if (! muted && muteInput && muted != lastMute)
        {
            // just became unmuted
            context.audio.applyGainRamp (0, numSamples, 0.0, node->getInputGain());
        }
        else if (node->getInputGain() != node->getLastInputGain())
        {
            context.audio.applyGainRamp (0, numSamples, node->getLastInputGain(), node->getInputGain());
        }
        else
        {
            context.audio.applyGain (0, numSamples, node->getInputGain());
        }

        for (int i = numAudioIns; --i >= 0;)
            node->setInputRMS (i, context.audio.getRMSLevel (i, 0, numSamples));

        // Begin MIDI filters
        {
            jassert (tempMidi.getNumEvents() == 0);
            ScopedLock spl (node->getPropertyLock());
            transpose.setNoteOffset (node->getTransposeOffset());
            const auto keyRange (node->getKeyRange());
            const auto midiChans (node->getMidiChannels());
            const auto useMidiProgram (node->areMidiProgramsEnabled());

            if (keyRange.getLength() > 0 || ! midiChans.isOmni() || useMidiProgram)
            {
                for (int i = 0; i < context.midi.getNumBuffers(); ++i)
                {
                    auto& midi = *context.midi.getWriteBuffer (i);
                    for (auto m : midi)
                    {
                        auto msg = m.getMessage();
                        if (msg.isNoteOnOrOff())
                        {
                            // out of range
                            if (keyRange.getLength() > 0 && (msg.getNoteNumber() < keyRange.getStart() || msg.getNoteNumber() > keyRange.getEnd()))
                                continue;
                        }

                        if (msg.getChannel() > 0 && midiChans.isOff (msg.getChannel()))
                            continue;

                        if (useMidiProgram && msg.isProgramChange())
                        {
                            node->setMidiProgram (msg.getProgramChangeNumber());
                            node->reloadMidiProgram();
                            continue;
                        }

                        transpose.process (msg);
                        tempMidi.addEvent (msg, m.samplePosition);
                    }

                    midi.swapWith (tempMidi);
                    tempMidi.clear();
                }
            }
            else
            {
                for (int i = context.midi.getNumBuffers(); --i >= 0;)
                    transpose.process (*context.midi.getWriteBuffer (i), numSamples);
            }
        }

        tempMidi.clear();
        // End MIDI filters

        auto pluginProcessBlock = [=] (RenderContext& context, bool isSuspended) {
            if (node->wantsContext())
            {
                if (! isSuspended)
                    node->render (context);
                else
                    node->renderBypassed (context);
            }
            else
            {
                jassert (processor != nullptr);
                if (! isSuspended)
                {
                    processor->processBlock (context.audio, *context.midi.getWriteBuffer (0));
                    // processor->processBlock (buffer, *sharedMidiBuffers.getUnchecked (midiBufferToUse));
                }
                else
                {
                    processor->processBlockBypassed (context.audio, *context.midi.getWriteBuffer (0));
                    // processor->processBlockBypassed (buffer, *sharedMidiBuffers.getUnchecked (midiBufferToUse));
                }
            }
        };

        const auto osFactor = node->getOversamplingFactor();
        if (osFactor > 1)
        {
            auto osProcessor = node->getOversamplingProcessor();

            dsp::AudioBlock<float> block (channels, static_cast<size_t> (totalChans), static_cast<size_t> (numSamples));
            dsp::AudioBlock<float> osBlock = osProcessor->processSamplesUp (block);

            if (totalChans > osChanSize)
            {
                osChanSize = context.audio.getNumChannels();
                osChans.reset (new float*[osChanSize]);
            }

            float** osData = osChans.get();
            for (int ch = 0; ch < totalChans; ++ch)
                osData[ch] = osBlock.getChannelPointer (ch);
            context.audio.setDataToReferTo (osData, totalChans, static_cast<int> (osBlock.getNumSamples()));

            tempMidi.clear();
            for (int i = 0; i < context.midi.getNumBuffers(); ++i)
            {
                auto& mb = *context.midi.getWriteBuffer (i);
                for (const MidiMessageMetadata msg : mb)
                {
                    tempMidi.addEvent (
                        msg.data,
                        msg.numBytes,
                        msg.samplePosition * osFactor);
                }
                mb.swapWith (tempMidi);
                tempMidi.clear();
            }

            pluginProcessBlock (context, node->isSuspended());

            osProcessor->processSamplesDown (block);
            for (int ch = 0; ch < totalChans; ++ch)
                osData[ch] = block.getChannelPointer (ch);
            context.audio.setDataToReferTo (osData, totalChans, numSamples);

            tempMidi.clear();
            for (int i = 0; i < context.midi.getNumBuffers(); ++i)
            {
                auto& mb = *context.midi.getWriteBuffer (i);
                for (const MidiMessageMetadata msg : mb)
                {
                    tempMidi.addEvent (
                        msg.data,
                        msg.numBytes,
                        msg.samplePosition / osFactor);
                }
                mb.swapWith (tempMidi);
                tempMidi.clear();
            }
        }
        else
        {
            pluginProcessBlock (context, node->isSuspended());
        }

        if (muted && ! muteInput)
        {
            if (lastMute != muted)
            {
                // just became muted
                context.audio.applyGainRamp (0, numSamples, node->getLastGain(), 0.0);
            }
            else
            {
                // normal mute processing
                context.audio.applyGain (0, numSamples, 0.0);
            }
        }
        else if (! muted && ! muteInput && muted != lastMute)
        {
            // just became unmuted
            context.audio.applyGainRamp (0, numSamples, 0.0, node->getGain());
        }
        else if (node->getGain() != node->getLastGain())
        {
            context.audio.applyGainRamp (0, numSamples, node->getLastGain(), node->getGain());
        }
        else
        {
            context.audio.applyGain (0, numSamples, node->getGain());
        }

        node->updateGain();
        lastMute = muted;

        for (int i = 0; i < numAudioOuts; ++i)
            node->setOutputRMS (i, context.audio.getRMSLevel (i, 0, numSamples));
    }

    const ProcessorPtr node;
    AudioProcessor* const processor;

private:
    Array<int> audioChannelsToUse;
    Array<int> cvChannelsToUse;
    Array<int> midiChannelsToUse;
    Array<int> atomChannelsToUse;

    HeapBlock<float*> channels;
    HeapBlock<float*> cv;
    int totalChans, totalCV, numAudioIns, numAudioOuts;
    int midiBufferToUse;
    bool lastMute = false;
    MidiTranspose transpose;
    MidiBuffer tempMidi;

    std::unique_ptr<float*> osChans;
    int osChanSize = 0;
    JUCE_DECLARE_NON_COPYABLE (ProcessBufferOp)
};

GraphBuilder::GraphBuilder (GraphNode& graph_,
                            const Array<void*>& orderedNodes_,
                            Array<void*>& renderingOps)
    : graph (graph_),
      orderedNodes (orderedNodes_),
      midi_MidiEvent (graph.symbols().map (LV2_MIDI__MidiEvent)),
      totalLatency (0)
{
    for (int i = 0; i < PortType::Unknown; ++i)
    {
        allNodes[i].add ((uint32) zeroNodeID); // first buffer is read-only zeros
        allPorts[i].add (EL_INVALID_PORT);
    }

    for (int i = 0; i < orderedNodes.size(); ++i)
    {
        createRenderingOpsForNode ((Processor*) orderedNodes.getUnchecked (i),
                                   renderingOps,
                                   i);
        markUnusedBuffersFree (i);
    }

#if EL_TRACE_GRAPH_OPS
    std::clog << "BEGIN\n";

    ProcessBufferOp* lastPbOp = nullptr;
    String nodeName;
    String stepsOutput;

    for (auto op : renderingOps)
    {
        auto obj = static_cast<GraphOp*> (op);

        if (auto rnd = dynamic_cast<ProcessBufferOp*> (obj))
        {
            if (rnd != lastPbOp)
            {
                String nodeName = rnd->node->getName();
                if (nodeName.isEmpty())
                    nodeName = "Unknown Node";
                lastPbOp = rnd;

                std::clog << "  " << nodeName << std::endl;
                std::clog << stepsOutput;
                stepsOutput.clear();
            }
            else
            {
                std::clog << "  -- proc step error --\n";
            }
            continue;
        }

        auto step = obj->traceStep();
        if (step.empty())
            stepsOutput << "    "
                        << "undefined step description";
        else
            stepsOutput << "    " << step;

        stepsOutput << "\n";
    }
    std::clog << "END" << std::endl;
#endif
}

int GraphBuilder::buffersNeeded (PortType _type)
{
    const auto type = _type == PortType::CV ? PortType::Audio : _type;
    return allNodes[type.id()].size();
}

int GraphBuilder::getNodeDelay (const uint32 nodeID) const { return nodeDelays[nodeDelayIDs.indexOf (nodeID)]; }

void GraphBuilder::setNodeDelay (const uint32 nodeID, const int latency)
{
    const int index = nodeDelayIDs.indexOf (nodeID);

    if (index >= 0)
    {
        nodeDelays.set (index, latency);
    }
    else
    {
        nodeDelayIDs.add (nodeID);
        nodeDelays.add (latency);
    }
}

int GraphBuilder::getInputLatency (const uint32 nodeID) const
{
    int maxLatency = 0;

    for (int i = graph.getNumConnections(); --i >= 0;)
    {
        const auto* const c = graph.getConnection (i);
        if (c->destNode == nodeID)
            maxLatency = jmax (maxLatency, getNodeDelay (c->sourceNode));
    }

    return maxLatency;
}

void GraphBuilder::createRenderingOpsForNode (Processor* const node,
                                              Array<void*>& renderingOps,
                                              const int ourRenderingIndex)
{
    AudioProcessor* const proc (node->getAudioProcessor());

    // don't add IONodes that cannot process
    if (IONode* ioproc = dynamic_cast<IONode*> (proc))
    {
        const uint32 numOuts = node->getNumPorts (PortType::Audio, false);
        if (IONode::audioInputNode == ioproc->getType() && numOuts <= 0)
            return;
        const uint32 numIns = node->getNumPorts (PortType::Audio, true);
        if (IONode::audioOutputNode == ioproc->getType() && numIns <= 0)
            return;
    }

    Array<int> channelsToUse[PortType::Unknown];
    int maxLatency = getInputLatency (node->nodeId);

    const uint32 numPorts (node->getNumPorts());
    for (uint32 port = 0; port < numPorts; ++port)
    {
        const PortType portType (node->getPortType (port));
        if (portType == PortType::Video || portType == PortType::Unknown || portType == PortType::Event)
            continue;

        const uint32 numIns = node->getNumPorts (portType, true);
        const uint32 numOuts = node->getNumPorts (portType, false);

        // Outputs only need a buffer if the channel index is greater
        // than or equal to the total inputs of the same port type
        if (node->isPortOutput (port))
        {
            switch (portType.id())
            {
                case PortType::Control: {
                    const int bufIndex = getFreeBuffer (portType);
                    markBufferAsContaining (bufIndex, portType, node->nodeId, port);
                    break;
                }

                default: {
                    const int outputChan = node->getChannelPort (port);
                    if (outputChan >= (int) numIns && outputChan < (int) numOuts)
                    {
                        const int bufIndex = getFreeBuffer (portType);
                        channelsToUse[portType.id()].add (bufIndex);
                        const uint32 outPort = node->getNthPort (portType, outputChan, false, false);

                        jassert (bufIndex != 0);
                        jassert (outPort == port);
                        jassert (outPort < node->getNumPorts());

                        markBufferAsContaining (bufIndex, portType, node->nodeId, outPort);
                    }
                    break;
                }
            }
            continue;
        }

        jassert (node->isPortInput (port));

        const int inputChan = node->getChannelPort (port);

        // get a list of all the inputs to this node
        Array<uint32> sourceNodes;
        Array<uint32> sourcePorts;
        Array<PortType> sourceTypes;

        for (int i = graph.getNumConnections(); --i >= 0;)
        {
            const auto* const c = graph.getConnection (i);
            if (c->destNode == node->nodeId && c->destPort == port)
            {
                sourceNodes.add (c->sourceNode);
                sourcePorts.add (c->sourcePort);
                auto src = graph.getNodeForId (c->sourceNode);
                sourceTypes.add (src->getPortType (c->sourcePort));
            }
        }

        int bufIndex = -1;
        PortType bufType = portType;

        if (sourceNodes.size() == 0)
        {
            // unconnected input channel
            if ((portType.isAudio() || portType.isCv()) && inputChan >= (int) numOuts)
            {
                bufIndex = getReadOnlyEmptyBuffer();
                jassert (bufIndex >= 0);
            }
            else
            {
                bufIndex = getFreeBuffer (portType);
                markBufferAsContaining (bufIndex, portType, anonymousNodeID, 0);

                switch (portType)
                {
                    case PortType::Audio:
                    case PortType::CV:
                        renderingOps.add (new ClearChannelOp (bufIndex));
                        break;
                    case PortType::Midi:
                        renderingOps.add (new ClearMidiBufferOp (bufIndex));
                        break;
                    case PortType::Atom:
                        renderingOps.add (new ClearAtomBufferOp (bufIndex));
                        break;
                    default:
                        break;
                }
            }
        }
        else if (sourceNodes.size() == 1)
        {
            // port with a straight forward single input..
            const uint32 srcNode = sourceNodes.getUnchecked (0);
            const uint32 srcPort = sourcePorts.getUnchecked (0);
            auto srcObj = graph.getNodeForId (srcNode);
            const auto srcType = srcObj->getPortType (srcPort);

            bufIndex = getBufferContaining (srcType, srcNode, srcPort);

            if (bufIndex < 0)
            {
                std::clog << "[element] feedback loop in one-to-one connection";
                // if not found, this is probably a feedback loop
                bufIndex = getReadOnlyEmptyBuffer();
                jassert (bufIndex >= 0);
            }

            const bool bufNeededLater = isBufferNeededLater (ourRenderingIndex, port, srcNode, srcPort);

            if (portType == PortType::Control)
            {
                auto src = graph.getNodeForId (srcNode);
                renderingOps.add (new BindParameterOp (
                    src->getParameter ((int) srcPort),
                    node->getParameter ((int) port)));
            }
            else if (srcType.isControl() && portType.isCv())
            {
                auto src = graph.getNodeForId (srcNode);
                const int newFreeBuffer = getFreeBuffer (portType);
                renderingOps.add (new ApplyParamToCVOp (src->getParameter ((int) srcPort), newFreeBuffer));
                bufIndex = newFreeBuffer;
            }
            // clang-format off
            else if (srcType != portType || 
                     (bufNeededLater && (inputChan < (int) numOuts || 
                                         portType == PortType::Midi || 
                                         portType == PortType::Atom)))
            // clang-format on
            {
                jassert (! portType.isControl());
                // can't mess up this channel because it's needed later by another node, so we
                // need to use a copy of it..
                const int newFreeBuffer = getFreeBuffer (portType);
                markBufferAsContaining (newFreeBuffer, portType, anonymousNodeID, 0);

                if (portType.isAudio() || portType.isCv())
                {
                    renderingOps.add (new CopyChannelOp (bufIndex, newFreeBuffer));
                }
                else if (srcType.isMidi() && portType.isMidi())
                {
                    renderingOps.add (new CopyMidiBufferOp (bufIndex, newFreeBuffer));
                }
                else if (srcType.isAtom() && portType.isAtom())
                {
                    renderingOps.add (new CopyAtomBufferOp (bufIndex, newFreeBuffer));
                }
                else if (srcType.isMidi() && portType.isAtom())
                {
                    renderingOps.add (new ClearAtomBufferOp (newFreeBuffer));
                    renderingOps.add (new MidiToAtomOp (bufIndex, newFreeBuffer));
                }
                else if (srcType.isAtom() && portType.isMidi())
                {
                    renderingOps.add (new ClearMidiBufferOp (newFreeBuffer));
                    renderingOps.add (new AtomToMidiOp (bufIndex, newFreeBuffer, midi_MidiEvent));
                }

                bufIndex = newFreeBuffer;
            }

            const int nodeDelay = getNodeDelay (srcNode);

            if (nodeDelay < maxLatency)
                renderingOps.add (new DelayChannelOp (bufIndex, maxLatency - nodeDelay));
        }
        else
        {
            // channel with a mix of several inputs..
            // try to find a re-usable channel from our inputs..
            int reusableInputIndex = -1;

            for (int i = 0; i < sourceNodes.size(); ++i)
            {
                if (! sourceTypes.getUnchecked (i).canConnect (portType))
                    continue;

                const int sourceBufIndex = getBufferContaining (sourceTypes.getUnchecked (i),
                                                                sourceNodes.getUnchecked (i),
                                                                sourcePorts.getUnchecked (i));

                if (sourceBufIndex >= 0
                    && ! isBufferNeededLater (ourRenderingIndex,
                                              port,
                                              sourceNodes.getUnchecked (i),
                                              sourcePorts.getUnchecked (i)))
                {
                    // we've found one of our input chans that can be re-used..
                    reusableInputIndex = i;
                    bufIndex = sourceBufIndex;

                    if (portType == PortType::Audio || portType == PortType::CV)
                    {
                        const int nodeDelay = getNodeDelay (sourceNodes.getUnchecked (i));
                        if (nodeDelay < maxLatency)
                            renderingOps.add (new DelayChannelOp (sourceBufIndex, maxLatency - nodeDelay));
                    }

                    break;
                }
            }

            if (reusableInputIndex < 0)
            {
                // can't re-use any of our input chans, so get a new one and copy everything into it..
                bufIndex = getFreeBuffer (portType);
                jassert (bufIndex != 0);

                markBufferAsContaining (bufIndex, portType, anonymousNodeID, 0);

                const int srcIndex = getBufferContaining (sourceTypes.getUnchecked (0),
                                                          sourceNodes.getUnchecked (0),
                                                          sourcePorts.getUnchecked (0));
                if (srcIndex < 0)
                {
                    // if not found, this is probably a feedback loop
                    std::clog << "[element] many-to-one feedback loop" << std::endl;
                    if (portType == PortType::Audio || portType == PortType::CV)
                        renderingOps.add (new ClearChannelOp (bufIndex));
                    else if (portType == PortType::Midi)
                        renderingOps.add (new ClearMidiBufferOp (bufIndex));
                    else if (portType == PortType::Atom)
                        renderingOps.add (new ClearAtomBufferOp (bufIndex));
                }
                else
                {
                    if (portType == PortType::Audio || portType == PortType::CV)
                    {
                        renderingOps.add (new CopyChannelOp (srcIndex, bufIndex));
                    }
                    else if (sourceTypes.getUnchecked (0).isMidi() && portType.isMidi())
                    {
                        renderingOps.add (new CopyMidiBufferOp (srcIndex, bufIndex));
                    }
                    else if (sourceTypes.getUnchecked (0).isAtom() && portType.isAtom())
                    {
                        renderingOps.add (new CopyAtomBufferOp (srcIndex, bufIndex));
                    }
                    else if (sourceTypes.getUnchecked (0).isAtom() && portType.isMidi())
                    {
                        renderingOps.add (new ClearMidiBufferOp (bufIndex));
                        renderingOps.add (new AtomToMidiOp (srcIndex, bufIndex, midi_MidiEvent));
                    }
                    else if (sourceTypes.getUnchecked (0).isMidi() && portType.isAtom())
                    {
                        renderingOps.add (new ClearAtomBufferOp (bufIndex));
                        renderingOps.add (new MidiToAtomOp (srcIndex, bufIndex));
                    }
                }

                reusableInputIndex = 0;

                if (portType == PortType::Audio)
                {
                    const int nodeDelay = getNodeDelay (sourceNodes.getFirst());
                    if (nodeDelay < maxLatency)
                        renderingOps.add (new DelayChannelOp (bufIndex, maxLatency - nodeDelay));
                }
            }

            for (int j = 0; j < sourceNodes.size(); ++j)
            {
                if (j != reusableInputIndex)
                {
                    int srcIndex = getBufferContaining (sourceTypes.getUnchecked (j),
                                                        sourceNodes.getUnchecked (j),
                                                        sourcePorts.getUnchecked (j));
                    if (srcIndex >= 0)
                    {
                        if (portType == PortType::Audio || portType == PortType::CV)
                        {
                            const int nodeDelay = getNodeDelay (sourceNodes.getUnchecked (j));

                            if (nodeDelay < maxLatency)
                            {
                                if (! isBufferNeededLater (ourRenderingIndex, port, sourceNodes.getUnchecked (j), sourcePorts.getUnchecked (j)))
                                {
                                    renderingOps.add (new DelayChannelOp (srcIndex, maxLatency - nodeDelay));
                                }
                                else // buffer is reused elsewhere, can't be delayed
                                {
                                    const int bufferToDelay = getFreeBuffer (PortType::Audio);
                                    renderingOps.add (new CopyChannelOp (srcIndex, bufferToDelay));
                                    renderingOps.add (new DelayChannelOp (bufferToDelay, maxLatency - nodeDelay));
                                    srcIndex = bufferToDelay;
                                }
                            }

                            renderingOps.add (new AddChannelOp (srcIndex, bufIndex));
                        }
                        else if (sourceTypes.getUnchecked (j).isMidi() && portType.isMidi())
                        {
                            renderingOps.add (new AddMidiBufferOp (srcIndex, bufIndex));
                        }
                        else if (sourceTypes.getUnchecked (j).isAtom() && portType.isAtom())
                        {
                            renderingOps.add (new AddAtomBufferOp (srcIndex, bufIndex));
                        }
                        else if (sourceTypes.getUnchecked (j).isAtom() && portType.isMidi())
                        {
                            renderingOps.add (new AtomToMidiOp (srcIndex, bufIndex, midi_MidiEvent));
                        }
                        else if (sourceTypes.getUnchecked (j).isMidi() && portType.isAtom())
                        {
                            renderingOps.add (new MidiToAtomOp (srcIndex, bufIndex));
                        }
                    }
                }
            }
        }

        jassert (bufIndex >= 0);
        channelsToUse[bufType.id()].add (bufIndex);

        if (inputChan < (int) numOuts)
        {
            const int outputPort = node->getNthPort (portType, inputChan, false, false);
            markBufferAsContaining (bufIndex, portType, node->nodeId, outputPort);
        }
    } /* foreach port */

    setNodeDelay (node->nodeId, maxLatency + node->getLatencySamples());

    if (node->isAudioIONode() && node->getNumPorts (PortType::Audio, false) == 0)
        totalLatency = maxLatency;

    int totalChans = jmax (node->getNumPorts (PortType::Audio, true),
                           node->getNumPorts (PortType::Audio, false));
    int totalCV = jmax (node->getNumPorts (PortType::CV, true),
                        node->getNumPorts (PortType::CV, false));
    renderingOps.add (new ProcessBufferOp (node, totalChans, totalCV, 0, channelsToUse));
}

int GraphBuilder::getFreeBuffer (PortType _type)
{
    jassert (_type.id() < PortType::Unknown);
    const PortType type = _type == PortType::CV ? PortType::Audio : _type;

    Array<uint32>& nodes = allNodes[type.id()];
    for (int i = 1; i < nodes.size(); ++i)
        if (nodes.getUnchecked (i) == freeNodeID)
            return i;

    nodes.add ((uint32) freeNodeID);
    return nodes.size() - 1;
}

int GraphBuilder::getReadOnlyEmptyBuffer() const noexcept { return 0; }

int GraphBuilder::getBufferContaining (const PortType _type, const uint32 nodeId, const uint32 outputPort) noexcept
{
    const PortType type = _type == PortType::CV ? PortType::Audio : _type;
    Array<uint32>& nodes = allNodes[type.id()];
    Array<uint32>& ports = allPorts[type.id()];

    for (int i = nodes.size(); --i >= 0;)
        if (nodes.getUnchecked (i) == nodeId
            && ports.getUnchecked (i) == outputPort)
            return i;

    return -1;
}

void GraphBuilder::markUnusedBuffersFree (const int stepIndex)
{
    for (const auto& type : PortType::all())
    {
        Array<uint32>& nodes = allNodes[type];
        Array<uint32>& ports = allPorts[type];

        for (int i = 0; i < nodes.size(); ++i)
        {
            if (isNodeBusy (nodes.getUnchecked (i))
                && ! isBufferNeededLater (stepIndex, EL_INVALID_PORT, nodes.getUnchecked (i), ports.getUnchecked (i)))
            {
                nodes.set (i, (uint32) freeNodeID);
            }
        }
    }
}

bool GraphBuilder::isBufferNeededLater (int stepIndexToSearchFrom,
                                        uint32 inputChannelOfIndexToIgnore,
                                        const uint32 sourceNode,
                                        const uint32 outputPortIndex) const
{
    while (stepIndexToSearchFrom < orderedNodes.size())
    {
        const Processor* const node = (const Processor*) orderedNodes.getUnchecked (stepIndexToSearchFrom);

        {
            for (uint32 port = 0; port < node->getNumPorts(); ++port)
            {
                if (port != inputChannelOfIndexToIgnore && graph.getConnectionBetween (sourceNode, outputPortIndex, node->nodeId, port) != nullptr)
                {
                    return true;
                }
            }
        }

        inputChannelOfIndexToIgnore = EL_INVALID_PORT;
        ++stepIndexToSearchFrom;
    }

    return false;
}

bool GraphBuilder::isBufferNeededLater2 (int stepIndexToSearchFrom,
                                         uint32 inputChannelOfIndexToIgnore,
                                         const uint32 sourceNode,
                                         const uint32 outputPortIndex) const
{
    while (stepIndexToSearchFrom < orderedNodes.size())
    {
        const Processor* const node = (const Processor*) orderedNodes.getUnchecked (stepIndexToSearchFrom);

        {
            for (uint32 port = 0; port < node->getNumPorts(); ++port)
            {
                if (port != inputChannelOfIndexToIgnore && graph.getConnectionBetween (sourceNode, outputPortIndex, node->nodeId, port) != nullptr)
                {
                    return true;
                }
            }
        }

        inputChannelOfIndexToIgnore = EL_INVALID_PORT;
        ++stepIndexToSearchFrom;
    }

    return false;
}

void GraphBuilder::markBufferAsContaining (int bufferNum, PortType _type, uint32 nodeId, uint32 portIndex)
{
    const PortType type = _type == PortType::CV ? PortType::Audio : _type;

    Array<uint32>& nodes = allNodes[type.id()];
    Array<uint32>& ports = allPorts[type.id()];

    jassert (bufferNum >= 0 && bufferNum < nodes.size());
    nodes.set (bufferNum, nodeId);
    ports.set (bufferNum, portIndex);
}

} // namespace element

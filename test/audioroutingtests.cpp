#include <boost/test/unit_test.hpp>

#include <element/context.hpp>
#include <element/symbolmap.hpp>

#include "fixture/PreparedGraph.h"
#include "fixture/TestNode.h"
#include "engine/graphnode.hpp"
#include "engine/ionode.hpp"
#include "utils.hpp"

using namespace element;

namespace {

/** Audio generator node that produces a test signal (ramp or sine wave) */
class AudioGeneratorNode : public TestNode {
public:
    enum WaveType { Ramp,
                    Sine,
                    DC };

    AudioGeneratorNode (int numChannels = 1, WaveType type = Ramp, float amplitude = 1.0f)
        : TestNode (0, numChannels, 0, 0), // audioIns, audioOuts, midiIns, midiOuts
          waveType (type),
          amp (amplitude),
          phase (0.0)
    {
    }

    void prepareToRender (double newSampleRate, int newBlockSize) override
    {
        TestNode::prepareToRender (newSampleRate, newBlockSize);
        phase = 0.0;
        phaseIncrement = 440.0 / newSampleRate; // 440 Hz for sine
    }

    void render (RenderContext& rc) override
    {
        const int numSamples = rc.audio.getNumSamples();
        const int numIns = getNumAudioInputs();
        const int numOuts = getNumAudioOutputs();
        const int numChans = jmin (numOuts, rc.audio.getNumChannels() - numIns);

        for (int ch = 0; ch < numChans; ++ch) {
            auto* output = rc.audio.getWritePointer (numIns + ch); // Outputs come after inputs
            double localPhase = phase;                             // Use local copy for per-channel phase

            for (int i = 0; i < numSamples; ++i) {
                float sample = 0.0f;

                switch (waveType) {
                    case Ramp:
                        sample = (float) i / (float) numSamples; // 0.0 to 1.0 ramp
                        break;

                    case Sine:
                        sample = std::sin (localPhase * MathConstants<double>::twoPi);
                        localPhase += phaseIncrement;
                        if (localPhase >= 1.0)
                            localPhase -= 1.0;
                        break;

                    case DC:
                        sample = 1.0f;
                        break;
                }

                output[i] = sample * amp; // Apply amplitude uniformly, no per-channel scaling
            }

            // Update phase for next block (only for sine wave, last channel)
            if (waveType == Sine && ch == numChans - 1) {
                phase = localPhase;
            }
        }
    }

private:
    WaveType waveType;
    float amp;
    double phase;
    double phaseIncrement;
};

/** Audio capture node that analyzes received audio */
class AudioCaptureNode : public TestNode {
public:
    AudioCaptureNode (int numChannels = 1)
        : TestNode (numChannels, 0, 0, 0) // audioIns, audioOuts, midiIns, midiOuts
    {
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 16; ++ch) {
            peakLevels[ch] = 0.0f;
            rmsLevels[ch] = 0.0f;
            sampleCount[ch] = 0;
        }
        totalSamples = 0;
        hasReceivedAudio = false;
    }

    void render (RenderContext& rc) override
    {
        const int numSamples = rc.audio.getNumSamples();
        const int numChans = jmin (rc.audio.getNumChannels(), getNumAudioInputs());

        if (numSamples > 0 && numChans > 0)
            hasReceivedAudio = true;

        for (int ch = 0; ch < numChans; ++ch) {
            const float* input = rc.audio.getReadPointer (ch);
            float peak = 0.0f;
            float sumSquares = 0.0f;

            for (int i = 0; i < numSamples; ++i) {
                const float sample = std::abs (input[i]);
                peak = jmax (peak, sample);
                sumSquares += input[i] * input[i];
            }

            peakLevels[ch] = jmax (peakLevels[ch], peak);
            rmsLevels[ch] = std::sqrt (sumSquares / numSamples);
            sampleCount[ch] += numSamples;
        }

        totalSamples += numSamples;
    }

    float getPeakLevel (int channel) const { return peakLevels[channel]; }
    float getRMSLevel (int channel) const { return rmsLevels[channel]; }
    int getSampleCount (int channel) const { return sampleCount[channel]; }
    bool hasAudio() const { return hasReceivedAudio; }

private:
    float peakLevels[16];
    float rmsLevels[16];
    int sampleCount[16];
    int totalSamples;
    bool hasReceivedAudio;
};

/** Audio passthrough node with gain control */
class AudioPassNode : public TestNode {
public:
    AudioPassNode (int numChannels = 1, float gainValue = 1.0f)
        : TestNode (numChannels, numChannels, 0, 0), // audioIns, audioOuts, midiIns, midiOuts
          gain (gainValue)
    {
    }

    void setGain (float newGain) { gain = newGain; }

    void render (RenderContext& rc) override
    {
        const int numSamples = rc.audio.getNumSamples();
        const int numIns = getNumAudioInputs();
        const int numOuts = getNumAudioOutputs();
        const int totalChans = rc.audio.getNumChannels();
        const int numChans = jmin (numIns, numOuts);

        // Bounds check to prevent crashes
        if (numIns + numOuts > totalChans)
            return;

        for (int ch = 0; ch < numChans; ++ch) {
            const float* input = rc.audio.getReadPointer (ch);
            float* output = rc.audio.getWritePointer (ch + numIns);

            for (int i = 0; i < numSamples; ++i) {
                output[i] = input[i] * gain;
            }
        }
    }

private:
    float gain;
};

/** Audio mixer node - sums all inputs to outputs */
class AudioMixerNode : public TestNode {
public:
    AudioMixerNode (int numInputs, int numOutputs)
        : TestNode (numInputs, numOutputs, 0, 0) // audioIns, audioOuts, midiIns, midiOuts
    {
        tempBuffer.setSize (1, 8192); // Max buffer size for mixing
    }

    void render (RenderContext& rc) override
    {
        const int numSamples = rc.audio.getNumSamples();
        const int numIns = getNumAudioInputs();
        const int numOuts = getNumAudioOutputs();
        const int totalChans = rc.audio.getNumChannels();

        // Simple bounds check
        if (totalChans < 1 || numSamples < 1)
            return;

        // Ensure temp buffer is large enough
        if (tempBuffer.getNumSamples() < numSamples)
            tempBuffer.setSize (1, numSamples, false, false, false);

        // For mixing with replace-processing: sum all inputs into temp buffer
        const int maxInputs = jmin(numIns, totalChans);
        const int outputChan = 0; // Write result to first channel (replace-processing)

        // Clear temp buffer
        float* temp = tempBuffer.getWritePointer(0);
        FloatVectorOperations::clear(temp, numSamples);
        
        // Sum all inputs into temp buffer
        for (int inCh = 0; inCh < maxInputs; ++inCh) {
            const float* input = rc.audio.getReadPointer (inCh);
            FloatVectorOperations::add (temp, input, numSamples);
        }
        
        // Copy result to output channel
        float* output = rc.audio.getWritePointer (outputChan);
        FloatVectorOperations::copy (output, temp, numSamples);
    }

private:
    AudioSampleBuffer tempBuffer;
};

} // anonymous namespace

BOOST_AUTO_TEST_SUITE (AudioRoutingTests)

BOOST_AUTO_TEST_CASE (SingleAudioToSingleAudio)
{
    // Basic test: single audio source -> single audio destination

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* gen = new AudioGeneratorNode (1, AudioGeneratorNode::Ramp, 0.5f);
    auto* capture = new AudioCaptureNode (1);
    graph.addNode (gen);
    graph.addNode (capture);
    BOOST_REQUIRE (gen != nullptr && capture != nullptr);

    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, capture->nodeId, 0));

    graph.rebuild();

    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify audio was received
    BOOST_REQUIRE (capture->hasAudio());
    BOOST_REQUIRE_GT (capture->getPeakLevel (0), 0.0f);
    BOOST_REQUIRE_EQUAL (capture->getSampleCount (0), 512);
}

BOOST_AUTO_TEST_CASE (MultiAudioToSingleAudio)
{
    // Test multiple audio sources merging into single destination
    // Should sum the signals

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* gen1 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.25f);
    auto* gen2 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.25f);
    auto* gen3 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.25f);
    auto* capture = new AudioCaptureNode (1);
    graph.addNode (gen1);
    graph.addNode (gen2);
    graph.addNode (gen3);
    graph.addNode (capture);

    BOOST_REQUIRE (gen1 != nullptr && gen2 != nullptr && gen3 != nullptr && capture != nullptr);

    // Connect all three generators to single capture input
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen1->nodeId, 0, capture->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen2->nodeId, 0, capture->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen3->nodeId, 0, capture->nodeId, 0));

    graph.rebuild();
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify signals merged (3 x 0.25 = ~0.75)
    BOOST_REQUIRE (capture->hasAudio());
    const float peak = capture->getPeakLevel (0);
    BOOST_REQUIRE_GT (peak, 0.7f);
    BOOST_REQUIRE_LT (peak, 0.8f);
}

BOOST_AUTO_TEST_CASE (SingleAudioToMultiAudio)
{
    // Test single audio source broadcasting to multiple destinations

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* gen = new AudioGeneratorNode (1, AudioGeneratorNode::Ramp, 0.5f);
    auto* capture1 = new AudioCaptureNode (1);
    auto* capture2 = new AudioCaptureNode (1);
    auto* capture3 = new AudioCaptureNode (1);

    graph.addNode (gen);
    graph.addNode (capture1);
    graph.addNode (capture2);
    graph.addNode (capture3);

    BOOST_REQUIRE (gen != nullptr && capture1 != nullptr && capture2 != nullptr && capture3 != nullptr);

    // Connect generator to all three captures
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, capture1->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, capture2->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, capture3->nodeId, 0));

    graph.rebuild();

    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify all captures received identical audio
    BOOST_REQUIRE (capture1->hasAudio() && capture2->hasAudio() && capture3->hasAudio());
    const float peak1 = capture1->getPeakLevel (0);
    const float peak2 = capture2->getPeakLevel (0);
    const float peak3 = capture3->getPeakLevel (0);

    BOOST_REQUIRE_GT (peak1, 0.0f);
    BOOST_REQUIRE_CLOSE (peak1, peak2, 0.01f);
    BOOST_REQUIRE_CLOSE (peak1, peak3, 0.01f);
}

BOOST_AUTO_TEST_CASE (AudioIsolation)
{
    // Test that disconnected audio paths don't interfere
    // Two separate gen->capture pairs should be independent

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* gen1 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.9f);

    graph.addNode (gen1);
    auto* gen2 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.1f);
    graph.addNode (gen2);
    auto* capture1 = new AudioCaptureNode (1);
    graph.addNode (capture1);
    auto* capture2 = new AudioCaptureNode (1);
    graph.addNode (capture2);

    BOOST_REQUIRE (gen1 != nullptr && gen2 != nullptr && capture1 != nullptr && capture2 != nullptr);

    // Create two isolated paths: gen1->capture1, gen2->capture2
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen1->nodeId, 0, capture1->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen2->nodeId, 0, capture2->nodeId, 0));

    graph.rebuild();
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify each capture only received its corresponding generator
    BOOST_REQUIRE (capture1->hasAudio() && capture2->hasAudio());
    const float peak1 = capture1->getPeakLevel (0);
    const float peak2 = capture2->getPeakLevel (0);

    BOOST_REQUIRE_GT (peak1, 0.8f);         // gen1 amplitude
    BOOST_REQUIRE_LT (peak2, 0.2f);         // gen2 amplitude
    BOOST_REQUIRE_GT (peak1 / peak2, 4.0f); // Should be ~9x ratio
}

BOOST_AUTO_TEST_CASE (DisconnectedAudioNode)
{
    // Test that disconnected audio node produces silence

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* gen = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 1.0f);

    graph.addNode (gen);
    auto* capture = new AudioCaptureNode (1);
    graph.addNode (capture);

    BOOST_REQUIRE (gen != nullptr && capture != nullptr);

    // Don't connect anything - capture should get silence

    graph.rebuild();
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify capture received silence
    BOOST_REQUIRE_EQUAL (capture->getPeakLevel (0), 0.0f);
}

BOOST_AUTO_TEST_CASE (AudioThroughIONodes)
{
    // Test audio routing through graph I/O nodes
    // External audio -> graph input -> processor -> graph output

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* audioIn = new IONode (IONode::audioInputNode);

    graph.addNode (audioIn);
    auto* audioOut = new IONode (IONode::audioOutputNode);
    graph.addNode (audioOut);
    auto* capture = new AudioCaptureNode (2);
    graph.addNode (capture);

    BOOST_REQUIRE (audioIn != nullptr && audioOut != nullptr && capture != nullptr);

    // Route: audioIn -> capture (verify external audio arrives)
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, audioIn->nodeId, 0, capture->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, audioIn->nodeId, 1, capture->nodeId, 1));

    graph.rebuild();

    // Create external audio input
    AudioSampleBuffer audio (2, 512);
    for (int ch = 0; ch < 2; ++ch) {
        auto* data = audio.getWritePointer (ch);
        for (int i = 0; i < 512; ++i)
            data[i] = 0.6f * (1.0f + ch * 0.5f); // Different level per channel
    }

    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify both channels received external audio
    BOOST_REQUIRE (capture->hasAudio());
    BOOST_REQUIRE_GT (capture->getPeakLevel (0), 0.5f);
    BOOST_REQUIRE_GT (capture->getPeakLevel (1), 0.8f); // Channel 1 has higher amplitude
}

BOOST_AUTO_TEST_CASE (ComplexAudioRouting)
{
    // Complex routing: multiple sources with different configurations
    // IONode audioIn + 2 generators -> 4 destinations with varied patterns

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* audioIn = new IONode (IONode::audioInputNode);

    graph.addNode (audioIn);
    auto* gen1 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.3f);
    graph.addNode (gen1);
    auto* gen2 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.5f);
    graph.addNode (gen2);
    auto* dst1 = new AudioCaptureNode (1);
    graph.addNode (dst1);
    auto* dst2 = new AudioCaptureNode (1);
    graph.addNode (dst2);
    auto* dst3 = new AudioCaptureNode (1);
    graph.addNode (dst3);
    auto* dst4 = new AudioCaptureNode (1);
    graph.addNode (dst4);

    BOOST_REQUIRE (audioIn != nullptr && gen1 != nullptr && gen2 != nullptr);
    BOOST_REQUIRE (dst1 != nullptr && dst2 != nullptr && dst3 != nullptr && dst4 != nullptr);

    // Complex routing patterns:
    // dst1 <- audioIn[0] only
    // dst2 <- gen1 + gen2
    // dst3 <- audioIn[0] + gen1
    // dst4 <- all three sources
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, audioIn->nodeId, 0, dst1->nodeId, 0));

    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen1->nodeId, 0, dst2->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen2->nodeId, 0, dst2->nodeId, 0));

    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, audioIn->nodeId, 0, dst3->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen1->nodeId, 0, dst3->nodeId, 0));

    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, audioIn->nodeId, 0, dst4->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen1->nodeId, 0, dst4->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen2->nodeId, 0, dst4->nodeId, 0));

    graph.rebuild();

    // External audio at 0.2 amplitude
    AudioSampleBuffer audio (2, 512);
    audio.clear();
    auto* data = audio.getWritePointer (0);
    for (int i = 0; i < 512; ++i)
        data[i] = 0.2f;

    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify each destination received correct sum
    BOOST_REQUIRE_CLOSE (dst1->getPeakLevel (0), 0.2f, 0.05f); // audioIn only
    BOOST_REQUIRE_CLOSE (dst2->getPeakLevel (0), 0.8f, 0.05f); // gen1(0.3) + gen2(0.5)
    BOOST_REQUIRE_CLOSE (dst3->getPeakLevel (0), 0.5f, 0.05f); // audioIn(0.2) + gen1(0.3)
    BOOST_REQUIRE_CLOSE (dst4->getPeakLevel (0), 1.0f, 0.05f); // all three
}

#if 1 // FIXME: This test has a crash issue
BOOST_AUTO_TEST_CASE (AudioChainWithBranching)
{
    // Test audio chain with branches
    // Gen1 -> Pass1 -> {Dst1, Dst2}
    // Gen2 -> Pass2 -> {Dst2, Dst3}
    // Dst2 receives both chains

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* gen1 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.3f);
    graph.addNode (gen1);
    std::cerr << "gen1=" << gen1 << std::endl;
    auto* gen2 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.4f);
    graph.addNode (gen2);
    std::cerr << "gen2=" << gen2 << std::endl;
    auto* pass1 = new AudioPassNode (1, 1.0f);
    graph.addNode (pass1);
    std::cerr << "pass1=" << pass1 << std::endl;
    auto* pass2 = new AudioPassNode (1, 1.0f);
    graph.addNode (pass2);
    std::cerr << "pass2=" << pass2 << std::endl;
    auto* dst1 = new AudioCaptureNode (1);
    graph.addNode (dst1);
    std::cerr << "dst1=" << dst1 << std::endl;
    auto* dst2 = new AudioCaptureNode (1);
    graph.addNode (dst2);
    std::cerr << "dst2=" << dst2 << std::endl;
    auto* dst3 = new AudioCaptureNode (1);
    graph.addNode (dst3);
    std::cerr << "dst3=" << dst3 << std::endl;

    BOOST_REQUIRE (gen1 != nullptr);
    BOOST_REQUIRE (gen2 != nullptr);
    BOOST_REQUIRE (pass1 != nullptr);
    BOOST_REQUIRE (pass2 != nullptr);
    BOOST_REQUIRE (dst1 != nullptr);
    BOOST_REQUIRE (dst2 != nullptr);
    BOOST_REQUIRE (dst3 != nullptr);

    // Build chain with branches
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen1->nodeId, 0, pass1->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, pass1->nodeId, 0, dst1->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, pass1->nodeId, 0, dst2->nodeId, 0));

    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen2->nodeId, 0, pass2->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, pass2->nodeId, 0, dst2->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, pass2->nodeId, 0, dst3->nodeId, 0));

    graph.rebuild();
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify branching worked correctly
    BOOST_REQUIRE_CLOSE (dst1->getPeakLevel (0), 0.3f, 0.05f); // gen1 only
    BOOST_REQUIRE_CLOSE (dst2->getPeakLevel (0), 0.7f, 0.05f); // gen1 + gen2
    BOOST_REQUIRE_CLOSE (dst3->getPeakLevel (0), 0.4f, 0.05f); // gen2 only
}
#endif

#if 1 // TODO: Fix AudioMultiLevelMerging - AudioPassNode doesn't sum inputs, needs proper mixer
BOOST_AUTO_TEST_CASE (AudioMultiLevelMerging)
{
    // Test hierarchical audio merging
    // Gen1 --\           /-> FinalDst1
    //         +-> Mix1 --
    // Gen2 --/           \-> FinalDst2
    //
    // Gen3 --\           /-> FinalDst2
    //         +-> Mix2 --
    // Gen4 --/           \-> FinalDst3

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* gen1 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.1f);

    graph.addNode (gen1);
    auto* gen2 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.2f);
    graph.addNode (gen2);
    auto* gen3 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.3f);
    graph.addNode (gen3);
    auto* gen4 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.4f);
    graph.addNode (gen4);
    auto* mix1 = new AudioPassNode (1, 1.0f);
    graph.addNode (mix1);
    auto* mix2 = new AudioPassNode (1, 1.0f);
    graph.addNode (mix2);
    auto* finalDst1 = new AudioCaptureNode (1);
    graph.addNode (finalDst1);
    auto* finalDst2 = new AudioCaptureNode (1);
    graph.addNode (finalDst2);
    auto* finalDst3 = new AudioCaptureNode (1);
    graph.addNode (finalDst3);

    BOOST_REQUIRE (gen1 && gen2 && gen3 && gen4 && mix1 && mix2);
    BOOST_REQUIRE (finalDst1 && finalDst2 && finalDst3);

    // First level merging
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen1->nodeId, 0, mix1->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen2->nodeId, 0, mix1->nodeId, 0));

    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen3->nodeId, 0, mix2->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen4->nodeId, 0, mix2->nodeId, 0));

    // Second level distribution
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, mix1->nodeId, 0, finalDst1->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, mix1->nodeId, 0, finalDst2->nodeId, 0));

    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, mix2->nodeId, 0, finalDst2->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, mix2->nodeId, 0, finalDst3->nodeId, 0));

    graph.rebuild();
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify hierarchical merging
    BOOST_REQUIRE_CLOSE (finalDst1->getPeakLevel (0), 0.3f, 0.05f); // gen1 + gen2
    BOOST_REQUIRE_CLOSE (finalDst2->getPeakLevel (0), 1.0f, 0.05f); // all four
    BOOST_REQUIRE_CLOSE (finalDst3->getPeakLevel (0), 0.7f, 0.05f); // gen3 + gen4
}
#endif

BOOST_AUTO_TEST_CASE (AudioIOPlusGenerators)
{
    // Critical test: Graph I/O audio input + internal generators -> multiple destinations
    // Tests that external audio and generated audio can coexist and route correctly

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* audioIn = new IONode (IONode::audioInputNode);

    graph.addNode (audioIn);
    auto* gen1 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.2f);
    graph.addNode (gen1);
    auto* gen2 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.3f);
    graph.addNode (gen2);
    auto* dst1 = new AudioCaptureNode (1);
    graph.addNode (dst1); // Gets audioIn + gen1
    auto* dst2 = new AudioCaptureNode (1);
    graph.addNode (dst2); // Gets gen2 only
    auto* dst3 = new AudioCaptureNode (1);
    graph.addNode (dst3); // Gets all sources

    BOOST_REQUIRE (audioIn && gen1 && gen2 && dst1 && dst2 && dst3);

    // dst1 <- audioIn[0] + gen1
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, audioIn->nodeId, 0, dst1->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen1->nodeId, 0, dst1->nodeId, 0));

    // dst2 <- gen2
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen2->nodeId, 0, dst2->nodeId, 0));

    // dst3 <- audioIn[0] + gen1 + gen2
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, audioIn->nodeId, 0, dst3->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen1->nodeId, 0, dst3->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen2->nodeId, 0, dst3->nodeId, 0));

    graph.rebuild();

    // External audio at 0.4 amplitude
    AudioSampleBuffer audio (2, 512);
    audio.clear();
    auto* data = audio.getWritePointer (0);
    for (int i = 0; i < 512; ++i)
        data[i] = 0.4f;

    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify routing with I/O + generators
    BOOST_REQUIRE_CLOSE (dst1->getPeakLevel (0), 0.6f, 0.05f); // audioIn(0.4) + gen1(0.2)
    BOOST_REQUIRE_CLOSE (dst2->getPeakLevel (0), 0.3f, 0.05f); // gen2 only
    BOOST_REQUIRE_CLOSE (dst3->getPeakLevel (0), 0.9f, 0.05f); // all three
}

BOOST_AUTO_TEST_CASE (AudioOutputIOUnused)
{
    // Test that unused audio output IO node doesn't interfere with signal chain

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* audioOut = new IONode (IONode::audioOutputNode);

    graph.addNode (audioOut); // Unused
    auto* gen = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.7f);
    graph.addNode (gen);
    auto* dst = new AudioCaptureNode (1);
    graph.addNode (dst);

    BOOST_REQUIRE (audioOut && gen && dst);

    // Connect gen -> dst only, audioOut unused
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, dst->nodeId, 0));

    graph.rebuild();
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify unused audio output doesn't interfere
    BOOST_REQUIRE_CLOSE (dst->getPeakLevel (0), 0.7f, 0.05f);
}

BOOST_AUTO_TEST_CASE (AudioOutputIOConnected)
{
    // Test audio output IO node with actual connections
    // Gen -> dst (internal capture) AND -> audioOut (external)

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* audioOut = new IONode (IONode::audioOutputNode);

    graph.addNode (audioOut);
    auto* gen = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.8f);
    graph.addNode (gen);
    auto* dst = new AudioCaptureNode (1);
    graph.addNode (dst);

    BOOST_REQUIRE (audioOut && gen && dst);

    // Connect gen -> dst (internal) AND gen -> audioOut (external)
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, dst->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, audioOut->nodeId, 0));

    graph.rebuild();
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify internal routing still works with audio output connected
    BOOST_REQUIRE_CLOSE (dst->getPeakLevel (0), 0.8f, 0.05f);
}

BOOST_AUTO_TEST_CASE (AudioFullIOChain)
{
    // Test full I/O chain with both input and output
    // audioIn -> processor -> audioOut
    // Also test multiple internal destinations don't interfere

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* audioIn = new IONode (IONode::audioInputNode);

    graph.addNode (audioIn);
    auto* audioOut = new IONode (IONode::audioOutputNode);
    graph.addNode (audioOut);
    auto* gen = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.5f);
    graph.addNode (gen);
    auto* dst1 = new AudioCaptureNode (1);
    graph.addNode (dst1);
    auto* dst2 = new AudioCaptureNode (1);
    graph.addNode (dst2);

    BOOST_REQUIRE (audioIn && audioOut && gen && dst1 && dst2);

    // Complex routing: gen -> {dst1, audioOut}, audioIn -> dst2
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, dst1->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, audioOut->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, audioIn->nodeId, 0, dst2->nodeId, 0));

    graph.rebuild();

    // External audio at 0.35 amplitude
    AudioSampleBuffer audio (2, 512);
    audio.clear();
    auto* data = audio.getWritePointer (0);
    for (int i = 0; i < 512; ++i)
        data[i] = 0.35f;

    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify routing: gen signal to dst1, audioIn signal to dst2
    BOOST_REQUIRE_CLOSE (dst1->getPeakLevel (0), 0.5f, 0.05f);  // gen
    BOOST_REQUIRE_CLOSE (dst2->getPeakLevel (0), 0.35f, 0.05f); // audioIn
}

BOOST_AUTO_TEST_CASE (AudioMultipleOutputs)
{
    // Test multiple audio output nodes (edge case)
    // Gen1 -> audioOut1, Gen2 -> {audioOut2, dst1}

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* audioOut1 = new IONode (IONode::audioOutputNode);

    graph.addNode (audioOut1);
    auto* audioOut2 = new IONode (IONode::audioOutputNode);
    graph.addNode (audioOut2);
    auto* gen1 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.6f);
    graph.addNode (gen1);
    auto* gen2 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.4f);
    graph.addNode (gen2);
    auto* dst1 = new AudioCaptureNode (1);
    graph.addNode (dst1);

    BOOST_REQUIRE (audioOut1 && audioOut2 && gen1 && gen2 && dst1);

    // Gen1 -> audioOut1 only
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen1->nodeId, 0, audioOut1->nodeId, 0));

    // Gen2 -> audioOut2 AND dst1
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen2->nodeId, 0, audioOut2->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen2->nodeId, 0, dst1->nodeId, 0));

    graph.rebuild();
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify dst1 only gets gen2 signal, not gen1
    BOOST_REQUIRE_CLOSE (dst1->getPeakLevel (0), 0.4f, 0.05f);
}

BOOST_AUTO_TEST_CASE (AudioStereoRouting)
{
    // Test stereo (2-channel) audio routing
    // Stereo generator -> stereo capture with independent channel verification

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* gen = new AudioGeneratorNode (2, AudioGeneratorNode::Ramp, 1.0f);

    graph.addNode (gen);
    auto* dst = new AudioCaptureNode (2);
    graph.addNode (dst);

    BOOST_REQUIRE (gen && dst);

    // Connect both channels
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, dst->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 1, dst->nodeId, 1));

    graph.rebuild();
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify both channels received audio
    BOOST_REQUIRE (dst->hasAudio());
    BOOST_REQUIRE_GT (dst->getPeakLevel (0), 0.9f);
    BOOST_REQUIRE_GT (dst->getPeakLevel (1), 0.9f);
    // Both channels should have identical amplitude (uniform scaling)
    BOOST_REQUIRE_CLOSE (dst->getPeakLevel (0), dst->getPeakLevel (1), 0.01f);
}

BOOST_AUTO_TEST_CASE (AudioChannelCrossover)
{
    // Test cross-channel routing patterns
    // Gen L->Dst R, Gen R->Dst L (channel swap)

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* gen = new AudioGeneratorNode (2, AudioGeneratorNode::DC, 1.0f);

    graph.addNode (gen);
    auto* dst = new AudioCaptureNode (2);
    graph.addNode (dst);

    BOOST_REQUIRE (gen && dst);

    // Swap channels: L->R, R->L
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, dst->nodeId, 1));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 1, dst->nodeId, 0));

    graph.rebuild();
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());

    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify crossover worked - channels should be swapped
    BOOST_REQUIRE (dst->hasAudio());
    const float peak0 = dst->getPeakLevel (0);
    const float peak1 = dst->getPeakLevel (1);
    // Both channels receive DC signal with same amplitude after swap
    BOOST_REQUIRE_CLOSE (peak0, 1.0f, 0.05f); // Received swapped gen channel 1
    BOOST_REQUIRE_CLOSE (peak1, 1.0f, 0.05f); // Received swapped gen channel 0
}

#if 1
BOOST_AUTO_TEST_CASE (AudioMixerNodeTest)
{
    // Test dedicated mixer node summing multiple inputs
    
    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* gen1 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.2f);

    graph.addNode (gen1);
    auto* gen2 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.3f);
    graph.addNode (gen2);
    auto* gen3 = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.25f);
    graph.addNode (gen3);
    auto* mixer = new AudioMixerNode (3, 1);
    graph.addNode (mixer);
    auto* dst = new AudioCaptureNode (1);
    graph.addNode (dst);

    BOOST_REQUIRE (gen1 && gen2 && gen3 && mixer && dst);

    // Connect all generators to mixer inputs
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen1->nodeId, 0, mixer->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen2->nodeId, 0, mixer->nodeId, 1));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen3->nodeId, 0, mixer->nodeId, 2));
    
    // Connect mixer output to destination
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, mixer->nodeId, 0, dst->nodeId, 0));

    graph.rebuild();
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    AtomBuffer atom;
    atom.setTypes (element::test::context()->symbols().mapPtr());
    
    RenderContext rc (audio, audio, midi, atom, 512);
    graph.render (rc);

    // Verify mixer summed all inputs (0.2 + 0.3 + 0.25 = 0.75)
    BOOST_REQUIRE_CLOSE (dst->getPeakLevel (0), 0.75f, 0.05f);
}
#endif

BOOST_AUTO_TEST_CASE (AudioFeedbackPrevention)
{
    // Test that graph prevents creating feedback loops
    // Attempt to create: Gen -> Pass -> Dst with Pass->Gen connection

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* gen = new AudioGeneratorNode (1, AudioGeneratorNode::DC, 0.5f);

    graph.addNode (gen);
    auto* pass = new AudioPassNode (1, 1.0f);
    graph.addNode (pass);
    auto* dst = new AudioCaptureNode (1);
    graph.addNode (dst);

    BOOST_REQUIRE (gen && pass && dst);

    // Create forward chain
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, gen->nodeId, 0, pass->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Audio, pass->nodeId, 0, dst->nodeId, 0));

    // Attempt feedback connection - should fail or be prevented
    // (Graph should detect cycle and reject connection)
    const bool feedbackAllowed = graph.connectChannels (PortType::Audio, pass->nodeId, 0, gen->nodeId, 0);

    // Graph should not allow feedback loops
    BOOST_REQUIRE (! feedbackAllowed);
}

BOOST_AUTO_TEST_SUITE_END()

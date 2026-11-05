// SPDX-FileCopyrightText: 2026 Kushview, LLC
// Author: jiri-god <318975419+jiri-god@users.noreply.github.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/test/unit_test.hpp>
#include "fixture/PreparedGraph.h"
#include "fixture/TestNode.h"
#include "engine/ionode.hpp"
#include "engine/miditranspose.hpp"
#include <array>
#include <cstring>
#if JUCE_MAC
#include <dlfcn.h>
#endif
using namespace element;
using namespace juce;
namespace {
#if JUCE_MAC
// Test-only observation of C malloc/free, including JUCE MidiMessage storage.
// Count this thread only; restore the previous logger after each test.
using MallocLogger = void (*) (uint32_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uint32_t);
thread_local bool watching = false;
thread_local unsigned allocations = 0, deallocations = 0;
MallocLogger previousLogger = nullptr;
void allocationLog (uint32_t type, uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t result, uint32_t skip)
{
    if (watching)
    {
        if (type & 2) ++allocations;
        if (type & 4) ++deallocations;
    }
    if (previousLogger) previousLogger (type, a, b, c, result, skip);
}
struct AllocationProbe
{
    MallocLogger* slot = reinterpret_cast<MallocLogger*> (dlsym (RTLD_DEFAULT, "malloc_logger"));
    AllocationProbe() { if (slot) { previousLogger = *slot; *slot = allocationLog; } }
    ~AllocationProbe() { watching = false; if (slot) *slot = previousLogger; }
    template <class F> std::pair<unsigned, unsigned> measure (F&& f)
    {
        allocations = deallocations = 0;
        watching = true;
        f();
        watching = false;
        return { allocations, deallocations };
    }
};
#endif
struct Sink : TestNode
{
    MidiBuffer received;
    Sink() : TestNode (0, 0, 1, 0) { received.ensureSize (65536); }
    void render (RenderContext& rc) override
    {
        received.clear();
        received.addEvents (*rc.midi.getWriteBuffer (0), 0, rc.audio.getNumSamples(), 0);
    }
};
std::array<uint8, 2048> packet()
{
    std::array<uint8, 2048> result {};
    result[0] = 0xf0; result[1] = 0x7d;
    for (size_t i = 2; i < result.size(); ++i) result[i] = uint8 (i % 128);
    return result;
}
void equalEvents (const MidiBuffer& actual, const MidiBuffer& expected)
{
    BOOST_REQUIRE_EQUAL (actual.getNumEvents(), expected.getNumEvents());
    auto it = actual.begin();
    for (const auto want : expected)
    {
        const auto got = *it++;
        BOOST_CHECK_EQUAL (got.samplePosition, want.samplePosition);
        BOOST_REQUIRE_EQUAL (got.numBytes, want.numBytes);
        BOOST_CHECK (std::memcmp (got.data, want.data, size_t (want.numBytes)) == 0);
    }
}
}
BOOST_AUTO_TEST_SUITE (MidiAllocationTests)
BOOST_AUTO_TEST_CASE (ForkedGraphPreservesPacketsFiltersAndReservedStorage)
{
    PreparedGraph fixture (48000, 32);
    auto& graph = fixture.graph;
    auto* input = graph.addNode (new IONode (IONode::midiInputNode));
    auto* a = new Sink; auto* b = new Sink;
    graph.addNode (a); graph.addNode (b);
    a->setTransposeOffset (12); a->setKeyRange (60, 70);
    BigInteger channels; channels.setBit (1); a->setMidiChannels (channels);
    b->setTransposeOffset (-12);
    BOOST_REQUIRE (graph.connectChannels (PortType::Midi, input->nodeId, 0, a->nodeId, 0));
    BOOST_REQUIRE (graph.connectChannels (PortType::Midi, input->nodeId, 0, b->nodeId, 0));
    graph.rebuild();
    AudioBuffer<float> audio (2, 32); audio.clear();
    MidiBuffer midi, wantA, wantB;
    midi.ensureSize (65536); wantA.ensureSize (65536); wantB.ensureSize (65536);
    RenderContext rc (audio, audio, midi, 32);
    const auto fill = [&] (int size)
    {
        midi.clear(); wantA.clear(); wantB.clear();
        if (size == 0) return;
        auto bytes = packet(); bytes[size_t (size - 1)] = 0xf7;
        for (auto* dst : { &midi, &wantA, &wantB }) dst->addEvent (bytes.data(), size, 0);
        midi.addEvent (MidiMessage::noteOn (1, 60, uint8 (100)), 2);
        midi.addEvent (MidiMessage::noteOn (1, 40, uint8 (100)), 3);
        midi.addEvent (MidiMessage::noteOn (2, 60, uint8 (100)), 4);
        wantA.addEvent (MidiMessage::noteOn (1, 72, uint8 (100)), 2);
        wantB.addEvent (MidiMessage::noteOn (1, 48, uint8 (100)), 2);
        wantB.addEvent (MidiMessage::noteOn (1, 28, uint8 (100)), 3);
        wantB.addEvent (MidiMessage::noteOn (2, 48, uint8 (100)), 4);
        for (auto* dst : { &midi, &wantA, &wantB })
        {
            dst->addEvent (MidiMessage::controllerEvent (1, 7, 99), 5);
            dst->addEvent (MidiMessage::midiClock(), 31);
        }
        for (int i = 0; i < 64; ++i)
            for (auto* dst : { &midi, &wantA, &wantB })
                dst->addEvent (MidiMessage::controllerEvent (1, 10, i), 16);
        midi.addEvent (MidiMessage::programChange (2, 3), 6);
        wantB.addEvent (MidiMessage::programChange (2, 3), 6);
    };
    // Establish capacity without requiring a particular graph reserve policy.
    // Empty/smaller blocks must not discard it and force later growth.
    for (int i = 0; i < 4; ++i) { fill (2048); graph.render (rc); }
#if JUCE_MAC
    AllocationProbe probe; BOOST_REQUIRE (probe.slot != nullptr);
    auto bytes = packet(); bytes.back() = 0xf7;
    int observedSize = 0;
    const auto positive = probe.measure ([&] { MidiMessage owning (bytes.data(), 2048); observedSize = owning.getRawDataSize(); });
    BOOST_CHECK_EQUAL (observedSize, 2048);
    BOOST_REQUIRE (positive.first > 0); BOOST_REQUIRE (positive.second > 0);
#endif
    for (int size : { 0, 10, 16, 22, 2048, 0, 2048 })
    {
        fill (size);
#if JUCE_MAC
        const auto counts = probe.measure ([&] { graph.render (rc); });
        BOOST_CHECK_EQUAL (counts.first, 0); BOOST_CHECK_EQUAL (counts.second, 0);
#else
        graph.render (rc);
#endif
        equalEvents (a->received, wantA); equalEvents (b->received, wantB);
    }
}
BOOST_AUTO_TEST_CASE (TransposeOnlyPreservesLongPacketsWithoutOwningCopies)
{
    MidiTranspose transpose; transpose.setNoteOffset (12);
    MidiBuffer midi, expected; midi.ensureSize (65536); expected.ensureSize (65536);
#if JUCE_MAC
    AllocationProbe probe; BOOST_REQUIRE (probe.slot != nullptr);
#endif
    for (int size : { 10, 16, 22, 2048, 10 })
    {
        midi.clear(); expected.clear();
        auto bytes = packet(); bytes[size_t (size - 1)] = 0xf7;
        midi.addEvent (bytes.data(), size, 5); expected.addEvent (bytes.data(), size, 5);
        midi.addEvent (MidiMessage::noteOff (1, 60), 31);
        expected.addEvent (MidiMessage::noteOff (1, 72), 31);
        midi.addEvent (MidiMessage::noteOn (1, 60, uint8 (100)), 32);
#if JUCE_MAC
        const auto counts = probe.measure ([&] { transpose.process (midi, 32); });
        BOOST_CHECK_EQUAL (counts.first, 0); BOOST_CHECK_EQUAL (counts.second, 0);
#else
        transpose.process (midi, 32);
#endif
        equalEvents (midi, expected);
    }
}
BOOST_AUTO_TEST_SUITE_END()

// SPDX-FileCopyrightText: 2026 Kushview, LLC
// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/test/unit_test.hpp>
#include <element/processor.hpp>
#include "engine/clapprovider.hpp"
using namespace element;
using namespace juce;
namespace {
struct TransportHead : AudioPlayHead {
    bool available = true;
    Optional<PositionInfo> getPosition() const override {
        if (!available) return nullopt;
        PositionInfo p; p.setBpm(123); p.setIsPlaying(true); return p;
    }
};
}
BOOST_AUTO_TEST_SUITE(ClapTransportTests)
BOOST_AUTO_TEST_CASE(RenderWithoutTransportAndAfterDetachment)
{
    CLAPProvider provider;
    std::unique_ptr<Processor> node(provider.create(String("net.kushview.test.transport:") + EL_TEST_TRANSPORT_PROBE));
    BOOST_REQUIRE(node != nullptr);
    node->prepareToRender(48000, 32);
    AudioBuffer<float> audio(2, 32); MidiBuffer midi;
    RenderContext rc(audio, audio, midi, 32);
    const auto render = [&](float expected) {
        audio.clear();
        for (int i=0; i<32; ++i) audio.setSample(0,i,float(i)/32);
        node->render(rc);
        for (int i=0; i<32; ++i) {
            BOOST_CHECK_EQUAL(audio.getSample(0,i),float(i)/32);
            BOOST_CHECK_EQUAL(audio.getSample(1,i),expected);
        }
    };
    render(-1); // No clock has ever been assigned: audio must still process.
    TransportHead head; node->setPlayHead(&head); render(123);
    head.available=false; render(-1); // No stale transport from previous block.
    head.available=true; render(123);
    node->setPlayHead(nullptr); render(-1); render(-1);
    node->setPlayHead(&head); render(123);
    node->releaseResources();
}
BOOST_AUTO_TEST_SUITE_END()

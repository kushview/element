
#include <boost/test/unit_test.hpp>

#include "engine/graphnode.hpp"
#include "nodes/audioprocessor.hpp"
#include "engine/ionode.hpp"
#include "engine/midichannelmap.hpp"
#include "nodes/midichannelmap.hpp"

using namespace element;

class MidiChannelMapUnit {
public:
    void initialise() {}

    void runTest()
    {
        testPortTypes();
        testPortConfig();
        testReset();
        testOutputCorrect();
        testProcess();
    }

private:
    void testProcess()
    {
        MidiChannelMap chmap;
        MidiMessage note (MidiMessage::noteOn (5, 100, 1.f));
        chmap.set (5, 6);
        chmap.process (note);
        BOOST_REQUIRE (chmap.get (5) == 6);
    }

    void testReset()
    {
        MidiChannelMap chmap;
        for (int ch = 1; ch <= 16; ++ch)
            chmap.set (ch, 17 - ch);
        chmap.reset();
        for (int ch = 1; ch <= 16; ++ch)
            BOOST_REQUIRE (chmap.get (ch) == ch);
    }

    void testOutputCorrect()
    {
        MidiChannelMap chmap;
        for (int ch = 1; ch <= 16; ++ch)
            BOOST_REQUIRE (chmap.get (ch) == ch);
        for (int ch = 1; ch <= 16; ++ch)
            chmap.set (ch, 17 - ch);
        for (int ch = 1; ch <= 16; ++ch)
            BOOST_REQUIRE (chmap.get (ch) == 17 - ch);
        for (int ch = 1; ch <= 16; ++ch)
            chmap.set (ch, 12);
        for (int ch = 1; ch <= 16; ++ch)
            BOOST_REQUIRE (chmap.get (ch) == 12);
    }

    void testPortTypes()
    {
        const PortType midi (String ("midi"));
        BOOST_REQUIRE (midi.isMidi());
        const PortType control (String ("control"));
        BOOST_REQUIRE (control.isControl());
    }

    void testPortConfig()
    {
        GraphNode proc;
        proc.setRenderDetails (44100.f, 512);
        proc.prepareToRender (44100.0, 512);
        auto* processor = new MidiChannelMapProcessor();
        BOOST_REQUIRE (processor->getTotalNumInputChannels() == 0);
        BOOST_REQUIRE (processor->getTotalNumOutputChannels() == 0);

        ProcessorPtr node = proc.addNode (new AudioProcessorNode (processor));
        MessageManager::getInstance()->runDispatchLoopUntil (10);
        BOOST_REQUIRE (processor->getTotalNumInputChannels() == 0);
        BOOST_REQUIRE (processor->getTotalNumOutputChannels() == 0);

        BOOST_REQUIRE (node != nullptr);

        BOOST_REQUIRE (node->getNumAudioInputs() == 0);
        BOOST_REQUIRE (node->getNumAudioOutputs() == 0);
        BOOST_REQUIRE (node->getNumPorts (PortType::Audio, true) == 0);
        BOOST_REQUIRE (node->getNumPorts (PortType::Audio, false) == 0);
        BOOST_REQUIRE (node->getNumPorts (PortType::Control, true) == 16);
        BOOST_REQUIRE (node->getNumPorts (PortType::Control, false) == 0);
        BOOST_REQUIRE (node->getNumPorts (PortType::Midi, true) == 1);
        BOOST_REQUIRE (node->getNumPorts (PortType::Midi, false) == 1);
        BOOST_REQUIRE (node->getNumPorts() == 18);

        for (int i = 0; i < 16; ++i) {
            BOOST_REQUIRE (node->getPortType (i) == PortType::Control);
            BOOST_REQUIRE (node->isPortInput (i));
        }

        BOOST_REQUIRE (node->getPortType (16) == PortType::Midi);
        BOOST_REQUIRE (node->isPortInput (16));
        BOOST_REQUIRE (node->getMidiInputPort() == 16);
        BOOST_REQUIRE (node->getPortType (17) == PortType::Midi);
        BOOST_REQUIRE (! node->isPortInput (17));
        BOOST_REQUIRE (node->getMidiOutputPort() == 17);

        BOOST_REQUIRE (node->getPortType (18) == PortType::Unknown);

        ProcessorPtr midiIn = proc.addNode (new element::IONode (element::IONode::midiInputNode));

        BOOST_REQUIRE (midiIn->getNumPorts() == 1);
        BOOST_REQUIRE (midiIn->getPortType (0) == PortType::Midi);
        BOOST_REQUIRE (midiIn->isPortInput (0) == false);
        BOOST_REQUIRE (proc.addConnection (midiIn->nodeId, midiIn->getMidiOutputPort(), node->nodeId, node->getMidiInputPort()));

        node = nullptr;
        midiIn = nullptr;
        proc.releaseResources();
        proc.clear();
    }
};

BOOST_AUTO_TEST_SUITE (MidiChannelMapTest)

BOOST_AUTO_TEST_CASE (Basics)
{
    MidiChannelMapUnit().runTest();
}

BOOST_AUTO_TEST_SUITE_END()

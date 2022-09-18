#include <boost/test/unit_test.hpp>
#include "engine/rootgraph.hpp"
#include "utils.hpp"

using namespace element;

struct DummyAudioDeviceSetup : DeviceManager::AudioDeviceSetup {
    explicit DummyAudioDeviceSetup (int numIns = 4, int numOuts = 4)
    {
        inputDeviceName = outputDeviceName = "Dummy";
        inputChannels.setRange (0, numIns, true);
        outputChannels.setRange (0, numOuts, true);
        sampleRate = 44100.0;
        bufferSize = 1024;
    }
};

BOOST_AUTO_TEST_SUITE (RootGraphTests)

BOOST_AUTO_TEST_CASE (Layout)
{
    RootGraph root;
    DummyAudioDeviceSetup setup;
    root.setPlayConfigFor (setup);
    BOOST_REQUIRE (root.getNumAudioInputs() == setup.inputChannels.countNumberOfSetBits());
    BOOST_REQUIRE (root.getNumAudioOutputs() == setup.outputChannels.countNumberOfSetBits());
    BOOST_REQUIRE (root.getNumPorts (PortType::Midi, true) == 1);
    BOOST_REQUIRE (root.getNumPorts (PortType::Midi, false) == 1);
    BOOST_REQUIRE (root.getName() == setup.inputDeviceName);
    BOOST_REQUIRE (root.getSampleRate() == setup.sampleRate);
    BOOST_REQUIRE (root.getBlockSize() == setup.bufferSize);
    root.clear();
}

BOOST_AUTO_TEST_SUITE_END()

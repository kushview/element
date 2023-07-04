
#include <boost/test/unit_test.hpp>
#include "engine/nodes/NodeTypes.h"
#include <element/nodefactory.hpp>

using namespace element;

BOOST_AUTO_TEST_SUITE (NodeFactoryTests)

BOOST_AUTO_TEST_CASE (Internals)
{
    NodeFactory nodes;
    const StringArray expectedIDs (
        EL_NODE_ID_AUDIO_ROUTER,
        EL_NODE_ID_GRAPH,
        EL_NODE_ID_MIDI_CHANNEL_SPLITTER,
        EL_NODE_ID_MIDI_MONITOR,
        EL_NODE_ID_MIDI_PROGRAM_MAP,
        EL_NODE_ID_MIDI_ROUTER,
        // EL_NODE_ID_MIDI_SEQUENCER,
        EL_NODE_ID_OSC_RECEIVER,
        EL_NODE_ID_OSC_SENDER,
        EL_NODE_ID_SCRIPT,
        EL_NODE_ID_MCU);

    OwnedArray<PluginDescription> types;
    for (const auto& ID : expectedIDs) {
        BOOST_REQUIRE (nodes.knownIDs().contains (ID));
        BOOST_REQUIRE (nullptr != std::unique_ptr<Processor> (nodes.instantiate (ID)));
        nodes.getPluginDescriptions (types, ID);
    }

    BOOST_REQUIRE (nodes.knownIDs().size() == expectedIDs.size());
    BOOST_REQUIRE (types.size() == expectedIDs.size());
    for (const auto* tp : types) {
        BOOST_REQUIRE (tp->name.isNotEmpty());
        BOOST_REQUIRE (expectedIDs.contains (tp->fileOrIdentifier));
        BOOST_REQUIRE (tp->pluginFormatName == EL_NODE_FORMAT_NAME);
    }

    nodes.hideType (EL_NODE_ID_MCU);
    BOOST_REQUIRE (nodes.isTypeHidden (EL_NODE_ID_MCU));
    nodes.removeHiddenType (EL_NODE_ID_MCU);
    BOOST_REQUIRE (! nodes.isTypeHidden (EL_NODE_ID_MCU));
}

BOOST_AUTO_TEST_SUITE_END()

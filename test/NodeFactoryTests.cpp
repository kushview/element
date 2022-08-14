
#include <boost/test/unit_test.hpp>
#include "engine/nodes/NodeTypes.h"
#include "engine/nodefactory.hpp"

using namespace Element;

BOOST_AUTO_TEST_SUITE (NodeFactoryTests)

BOOST_AUTO_TEST_CASE (Internals)
{
    NodeFactory nodes;
    const StringArray expectedIDs (
        EL_INTERNAL_ID_AUDIO_ROUTER,
        EL_INTERNAL_ID_GRAPH,
        EL_INTERNAL_ID_LUA,
        EL_INTERNAL_ID_MIDI_CHANNEL_SPLITTER,    
        EL_INTERNAL_ID_MIDI_MONITOR,            
        EL_INTERNAL_ID_MIDI_PROGRAM_MAP,        
        EL_INTERNAL_ID_MIDI_ROUTER,           
        // EL_INTERNAL_ID_MIDI_SEQUENCER,        
        EL_INTERNAL_ID_OSC_RECEIVER,           
        EL_INTERNAL_ID_OSC_SENDER,            
        EL_INTERNAL_ID_SCRIPT              
    );

    OwnedArray<PluginDescription> types;
    for (const auto& ID : expectedIDs)
    {
        BOOST_REQUIRE (nodes.getKnownIDs().contains (ID));
        BOOST_REQUIRE (nullptr != std::unique_ptr<NodeObject> (nodes.instantiate (ID)));
        nodes.getPluginDescriptions (types, ID);
    }

    BOOST_REQUIRE (nodes.getKnownIDs().size() == expectedIDs.size());
    BOOST_REQUIRE (types.size() == expectedIDs.size());
    for (const auto* tp : types)
    {
        BOOST_REQUIRE (tp->name.isNotEmpty());
        BOOST_REQUIRE (expectedIDs.contains (tp->fileOrIdentifier));
        BOOST_REQUIRE (tp->pluginFormatName == EL_INTERNAL_FORMAT_NAME);
    }
}

BOOST_AUTO_TEST_SUITE_END()

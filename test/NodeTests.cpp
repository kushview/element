
#include <boost/test/unit_test.hpp>
#include <element/node.hpp>

using namespace element;

BOOST_AUTO_TEST_SUITE (NodeTests)

BOOST_AUTO_TEST_CASE (Ctors)
{
    Node node;
    BOOST_REQUIRE (! node.isValid());
    node = element::Node (types::Node);
    BOOST_REQUIRE (node.getName().isNotEmpty());
    BOOST_REQUIRE (node.getNodeType() == types::Node);
    BOOST_REQUIRE (node.getUuidString().isNotEmpty());
    BOOST_REQUIRE (! node.getUuid().isNull());

    node = element::Node (node.data().createCopy(), false);
    BOOST_REQUIRE (node.getName().isNotEmpty());
    BOOST_REQUIRE (node.getNodeType() == types::Node);
    BOOST_REQUIRE (node.getUuidString().isNotEmpty());
    BOOST_REQUIRE (! node.getUuid().isNull());
}

BOOST_AUTO_TEST_CASE (DefaultGraph)
{
    auto node = Node::createDefaultGraph ("CustomName");
    BOOST_REQUIRE_EQUAL (node.getName().toStdString(), "CustomName");
    BOOST_REQUIRE_EQUAL (node.getNumNodes(), 4);
    for (int i = 0; i < node.getNumNodes(); ++i)
        BOOST_REQUIRE (! node.getNode (i).getUuid().isNull());
    node = Node::createDefaultGraph();
    BOOST_REQUIRE (node.getName().isEmpty());
}

BOOST_AUTO_TEST_CASE (HiddenBlockPorts)
{
    Node node (types::Node);
    auto ports = node.getPortsValueTree();
    for (int i = 0; i < 16; ++i) {
        Port port;
        auto vtPort = port.data();
        vtPort.setProperty (tags::index, i, nullptr)
            .setProperty (tags::type, "audio", nullptr)
            .setProperty (tags::symbol, String ("symbol_in_") + String (i), nullptr)
            .setProperty (tags::flow, "input", nullptr);
        ports.appendChild (vtPort, nullptr);
    }

    for (int i = 16; i < 32; ++i) {
        Port port;
        auto vtPort = port.data();
        vtPort.setProperty (tags::index, i, nullptr)
            .setProperty (tags::type, "audio", nullptr)
            .setProperty (tags::symbol, String ("symbol_out_") + String (i - 16), nullptr)
            .setProperty (tags::flow, "output", nullptr);
        ports.appendChild (vtPort, nullptr);
    }

    for (int i = 8; i < 16; ++i) {
        auto port = node.getPort (i);
        port.setHiddenOnBlock (true);
    }

    auto port = node.getPort (0);
    std::clog << port.toXmlString().toStdString() << std::endl;
    std::clog << port.getNode().toXmlString().toStdString() << std::endl;

    BOOST_REQUIRE_MESSAGE (! port.isHiddenOnBlock(), "Unmodified should not be hidden on block");

    port = node.getPort (10);
    BOOST_REQUIRE_MESSAGE (port.isHiddenOnBlock(),
                           port.symbol().toStdString() + std::string (" Should be hidden on block"));
    port.setHiddenOnBlock (false);
    BOOST_REQUIRE_MESSAGE (! port.isHiddenOnBlock(), "Modified should not be hidden on block");
}

BOOST_AUTO_TEST_SUITE_END()

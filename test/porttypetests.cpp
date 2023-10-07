
#include <boost/test/unit_test.hpp>
#include <element/porttype.hpp>

using element::PortType;

BOOST_AUTO_TEST_SUITE (PortTypeTests)

BOOST_AUTO_TEST_CASE (all)
{
    BOOST_REQUIRE_EQUAL (PortType::all().size(), PortType::Unknown);
}

BOOST_AUTO_TEST_SUITE_END()

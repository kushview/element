#define BOOST_TEST_MODULE Element
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Element)

BOOST_AUTO_TEST_CASE (Sanity) {
    BOOST_TEST_REQUIRE(2 + 2 != 5);
}

BOOST_AUTO_TEST_SUITE_END()

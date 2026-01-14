// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#define BOOST_TEST_MODULE Element
#include <boost/test/included/unit_test.hpp>
#include <element/juce.hpp>
using namespace juce;

#include <element/context.hpp>
#include <element/services.hpp>

namespace element {
namespace test {
static std::unique_ptr<Context> _context;
Context* context()
{
    if (_context == nullptr) {
        _context = std::make_unique<Context> (RunMode::Standalone);
        _context->services().initialize();
        _context->services().activate();
    }
    return _context.get();
}

void resetContext()
{
    _context.reset();
}
} // namespace test
} // namespace element

struct JuceMessageManagerFixture {
    JuceMessageManagerFixture()
    {
        MessageManager::getInstance();
        juce::initialiseJuce_GUI();
    }

    ~JuceMessageManagerFixture()
    {
        element::test::resetContext();
        juce::shutdownJuce_GUI();
    }
};

BOOST_GLOBAL_FIXTURE (JuceMessageManagerFixture);

BOOST_AUTO_TEST_SUITE (Element)

BOOST_AUTO_TEST_CASE (Sanity)
{
    BOOST_TEST_REQUIRE (2 + 2 != 5);
}

BOOST_AUTO_TEST_SUITE_END()

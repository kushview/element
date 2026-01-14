// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <element/session.hpp>

using namespace element;

BOOST_AUTO_TEST_SUITE (SessionChangedTest)

BOOST_AUTO_TEST_CASE (HasChanged)
{
#if 0
    // TODO: need a fixtrure
    beginTest ("not flagged changed after session open and save");
    const auto sessionFile = getDataDir().getChildFile ("Sessions/Default.els");
    expect (sessionFile.existsAsFile());
    auto* const controller = services().find<SessionService>();
    controller->openFile (sessionFile);
    runDispatchLoop (40);
    expect (! controller->hasSessionChanged());
    controller->saveSession (false);
    runDispatchLoop (40);
    expect (! controller->hasSessionChanged());
#endif
}

BOOST_AUTO_TEST_SUITE_END()

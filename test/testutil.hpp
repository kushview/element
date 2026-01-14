// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/context.hpp>

namespace element {
namespace test {

/** Returns a global element::Context. */
Context* context();

/** Deletes the global context.
    calling test::context() will re-instantiate it
 */
void resetContext();

/** Returns the toplevel source directory. */
inline static juce::File sourceRoot()
{
#ifdef EL_TEST_SOURCE_ROOT
    return juce::File (EL_TEST_SOURCE_ROOT);
#else
    return juce::File::getCurrentWorkingDirectory().getParentDirectory();
#endif
}

} // namespace test
} // namespace element

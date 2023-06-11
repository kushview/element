#pragma once

namespace element {
namespace test {

inline static juce::File getSourceRoot()
{
#ifdef EL_TEST_SOURCE_ROOT
    return juce::File (EL_TEST_SOURCE_ROOT);
#else
    return juce::File::getCurrentWorkingDirectory().getParentDirectory();
#endif
}

} // namespace test
} // namespace element

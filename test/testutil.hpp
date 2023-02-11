#pragma once

namespace element {
namespace test {

inline static File getSourceRoot()
{
#ifdef EL_TEST_SOURCE_ROOT
    return File (EL_TEST_SOURCE_ROOT);
#else
    return File::getCurrentWorkingDirectory().getParentDirectory();
#endif
}

} // namespace test
} // namespace element

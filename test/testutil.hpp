#pragma once

namespace element {
namespace test {

inline static File getSourceRoot()
{
    return File::getCurrentWorkingDirectory().getParentDirectory();
}

} // namespace test
} // namespace element

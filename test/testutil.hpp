#pragma once

namespace Element {
namespace test {

inline static File getSourceRoot()
{
    return File::getCurrentWorkingDirectory().getParentDirectory();
}

} // namespace test
} // namespace Element

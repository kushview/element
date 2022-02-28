#pragma once

// clang-format off

#if __linux__ || defined(__MINGW32__)
 #if __GNUC__ > 7
  #include <filesystem>
 #else
  #include <experimental/filesystem>
  namespace std { namespace filesystem = experimental::filesystem; }
 #endif

#elif defined(__APPLE__)
 #if __MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_14
  #include <filesystem>
 #else
  #include <experimental/filesystem>
  namespace std { namespace filesystem = experimental::filesystem; }
 #endif
#endif

// clang-format on

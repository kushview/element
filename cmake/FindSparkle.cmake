if(APPLE)    
    set(SPARKLE_VERSION "2.8.1" CACHE STRING "Sparkle version to download")
    
    FetchContent_Declare(
        sparkle
        URL https://github.com/Sparkle-project/Sparkle/releases/download/${SPARKLE_VERSION}/Sparkle-${SPARKLE_VERSION}.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    
    FetchContent_MakeAvailable(sparkle)
    message(STATUS "Searching for Sparkle in: ${sparkle_SOURCE_DIR}")
    find_library(SPARKLE_FRAMEWORK Sparkle HINTS ${sparkle_SOURCE_DIR})
    if(SPARKLE_FRAMEWORK)
        message(STATUS "Found Sparkle: ${SPARKLE_FRAMEWORK}")
    else()
        message(FATAL_ERROR "Sparkle framework not found")
    endif()
endif()

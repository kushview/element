find_package(PkgConfig)
if(PkgConfig_FOUND)
    message(STATUS "Checking for jack audio")
    pkg_check_modules(JACK jack)
    if(JACK_FOUND)
        message(STATUS "Found jack with pkg-config")
    endif()
else()
    # pkg config not found, check for jack headers and create JACK_INCLUDE_DIRS
    message(STATUS "Checking for jack audio headers")
    
    if(WIN32)
        # On Windows, check standard JACK2 installation paths
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # 64-bit target
            set(JACK_SEARCH_PATHS "C:/Program Files/JACK2/include")
        else()
            # 32-bit target
            set(JACK_SEARCH_PATHS "C:/Program Files (x86)/JACK2/include")
        endif()
    endif()
    
    # Find jack headers
    find_path(JACK_INCLUDE_DIR
        NAMES jack/jack.h jack/weakjack.h
        PATHS ${JACK_SEARCH_PATHS}
        PATH_SUFFIXES include
    )
    
    if(JACK_INCLUDE_DIR)
        set(JACK_INCLUDE_DIRS ${JACK_INCLUDE_DIR})
        set(JACK_FOUND TRUE)
        message(STATUS "Found JACK headers at ${JACK_INCLUDE_DIR}")
    else()
        set(JACK_FOUND FALSE)
        message(STATUS "JACK headers not found")
    endif()
endif()

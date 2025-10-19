# Generate a version header in a specific path (build/include/element/version.h)
execute_process(COMMAND git rev-list --count HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE EL_BUILD_NUMBER
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git rev-parse --verify HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE EL_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT EL_GIT_HASH)
    set(EL_GIT_HASH "unknown")
endif()
string(SUBSTRING ${EL_GIT_HASH} 0 8 EL_GIT_SHORT_HASH)
set(EL_VERSION_STRING ${PROJECT_VERSION})

set(GENERATED_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include/element)
file(MAKE_DIRECTORY ${GENERATED_INCLUDE_DIR})

# version.h.in should live in the source tree (see next block)
configure_file(${CMAKE_SOURCE_DIR}/cmake/version.h.in
    ${GENERATED_INCLUDE_DIR}/version.h @ONLY)

target_include_directories(element PRIVATE ${GENERATED_INCLUDE_DIR})

# Lua library configuration
set(LUA_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Platform-specific C arguments
set(LUA_C_ARGS "")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND LUA_C_ARGS "-DLUA_USE_LINUX")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND LUA_C_ARGS "-DLUA_USE_MACOSX")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # not actually needed
    # list(APPEND LUA_C_ARGS "-DLUA_USE_WINDOWS")
endif()

# Create static library
add_library(element_lua STATIC
    src/lauxlib.c
    src/liolib.c
    src/lopcodes.c
    src/lstate.c
    src/lobject.c
    src/lmathlib.c
    src/loadlib.c
    src/lvm.c
    src/lfunc.c
    src/lstrlib.c
    src/linit.c
    src/lstring.c
    src/lundump.c
    src/lctype.c
    src/ltable.c
    src/ldump.c
    src/loslib.c
    src/lgc.c
    src/lzio.c
    src/ldblib.c
    src/lutf8lib.c
    src/lmem.c
    src/lcorolib.c
    src/lcode.c
    src/ltablib.c
    src/lapi.c
    src/lbaselib.c
    src/ldebug.c
    src/lparser.c
    src/llex.c
    src/ltm.c
    src/ldo.c
)

target_include_directories(element_lua PRIVATE ${LUA_INCLUDES})
target_compile_options(element_lua PRIVATE ${LUA_C_ARGS})

# Set symbol visibility to default (equivalent to gnu_symbol_visibility : 'default')
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(element_lua PRIVATE -fvisibility=default)
endif()

# Don't install this library (install : false in meson)
set_target_properties(element_lua PROPERTIES
    EXCLUDE_FROM_ALL FALSE
    EXCLUDE_FROM_DEFAULT_BUILD FALSE
)

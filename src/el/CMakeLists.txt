file(GLOB ELEMENT_LUA_MODS "${CMAKE_CURRENT_SOURCE_DIR}/*.lua")
juce_add_binary_data(element-luamods
    HEADER_NAME luamods.hpp 
    NAMESPACE mods
    SOURCES ${ELEMENT_LUA_MODS})

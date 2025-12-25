file(GLOB ELEMENT_LUA_MODS "${CMAKE_CURRENT_SOURCE_DIR}/*.lua")
# Lua scripts are currently built into the the binaries until the app and plugins
# are able to deal with search paths and so forth.
if(TRUE)
    juce_add_binary_data(element-luamods
        HEADER_NAME luamods.hpp 
        NAMESPACE mods
        SOURCES ${ELEMENT_LUA_MODS})
else()
    install(FILES ${ELEMENT_LUA_MODS} 
        DESTINATION ${CMAKE_INSTALL_DATADIR}/element/lua)
endif()

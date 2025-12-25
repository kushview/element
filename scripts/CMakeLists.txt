file(GLOB ELEMENT_LUA_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/*.lua")
# Lua scripts are currently built into the the binaries until the app and plugins
# are able to deal with search paths and so forth.
if(TRUE)
    juce_add_binary_data(element-luascripts 
        HEADER_NAME luascripts.hpp 
        NAMESPACE scripts
        SOURCES ${ELEMENT_LUA_SCRIPTS})
else()
    install(FILES ${ELEMENT_LUA_SCRIPTS} 
        DESTINATION ${CMAKE_INSTALL_DATADIR}/element/scripts)
endif()

file(GLOB ELEMENT_LUA_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/*.lua")
juce_add_binary_data(element-luascripts 
    HEADER_NAME luascripts.hpp 
    NAMESPACE scripts
    SOURCES ${ELEMENT_LUA_SCRIPTS})

install(FILES ${ELEMENT_LUA_SCRIPTS} 
    DESTINATION ${CMAKE_INSTALL_DATADIR}/element/scripts)

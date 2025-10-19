file(GLOB_RECURSE ELEMENT_RES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
juce_add_binary_data(element-binarydata 
    HEADER_NAME binarydata.hpp 
    NAMESPACE element::res
    SOURCES ${ELEMENT_RES})

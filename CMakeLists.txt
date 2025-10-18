# Element CMakeLists.txt

cmake_minimum_required(VERSION 3.22)
project(element VERSION 1.0.0)

add_subdirectory(subprojects/juce EXLUDE_FROM_ALL)

juce_add_gui_app(element
    # VERSION ...                       # Set this if the app version is different to the project version
    # ICON_BIG ...                      # ICON_* arguments specify a path to an image file to use as an icon
    # ICON_SMALL ...
    # DOCUMENT_EXTENSIONS ...           # Specify file extensions that should be associated with this app
    COMPANY_NAME "Kushview"             # Specify the name of the app's author
    PRODUCT_NAME "Element")             # The name of the final executable, which can differ from the target name

add_subdirectory(src/lua)

target_include_directories(element PUBLIC include)
target_include_directories(element PRIVATE src)
file(GLOB_RECURSE ELEMENT_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

target_sources(element PRIVATE ${ELEMENT_SOURCES})

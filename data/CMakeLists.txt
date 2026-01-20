# SPDX-FileCopyrightText: 2026 Kushview, LLC
# SPDX-License-Identifier: GPL-3.0-or-later

file(GLOB_RECURSE ELEMENT_RES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
juce_add_binary_data(element-binarydata 
    HEADER_NAME binarydata.hpp 
    NAMESPACE element::res
    SOURCES ${ELEMENT_RES})

# Linux desktop file and icon installation
if(UNIX AND NOT APPLE)
    # Configure desktop file with CMake variables
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/element.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/element.desktop
        @ONLY
    )

    # Install desktop file
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/element.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )

    # Install icon
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/images/icon.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps
        RENAME element.png
    )
endif()
